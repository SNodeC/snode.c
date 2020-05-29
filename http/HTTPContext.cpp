#include "HTTPContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <filesystem>
#include <string.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPStatusCodes.h"
#include "MimeTypes.h"
#include "SocketConnection.h"
#include "WebApp.h"
#include "httputils.h"


HTTPContext::HTTPContext(WebApp* httpServer, SocketConnection* connectedSocket)
    : connectedSocket(connectedSocket)
    , webApp(httpServer)
    , request(this)
    , response(this) {
    this->responseStatus = 200;
    this->requestState = requeststates::REQUEST;
    this->lineState = linestate::READ;
    this->bodyData = 0;
    this->bodyLength = 0;
    this->bodyPointer = 0;
    this->headerSend = false;
}


void HTTPContext::receiveRequest(const char* junk, ssize_t junkLen) {
    parseRequest(
        junk, junkLen,
        [&](const std::string& line) -> void { // header data
            switch (requestState) {
            case requeststates::REQUEST:
                if (!line.empty()) {
                    parseRequestLine(line);
                    requestState = requeststates::HEADER;
                } else {
                    this->responseStatus = 400;
                    this->responseHeader.insert({"Connection", "Close"});
                    this->end();
                    connectedSocket->end();
                    requestState = requeststates::ERROR;
                }
                break;
            case requeststates::HEADER:
                if (!line.empty()) {
                    this->addRequestHeader(line);
                } else {
                    if (bodyLength != 0) {
                        requestState = requeststates::BODY;
                    } else {
                        this->requestReady();
                        requestState = requeststates::REQUEST;
                    }
                }
                break;
            case requeststates::BODY:
            case requeststates::ERROR:
                break;
            }
        },
        [&](const char* bodyJunk, int junkLen) -> void { // body data
            if (bodyLength - bodyPointer < junkLen) {
                junkLen = bodyLength - bodyPointer;
            }
            memcpy(bodyData + bodyPointer, bodyJunk, junkLen);
            bodyPointer += junkLen;
            if (bodyPointer == bodyLength) {
                this->requestReady();
            }
        });
}


void HTTPContext::parseRequest(const char* junk, ssize_t junkLen, const std::function<void(std::string&)>& lineRead,
                               const std::function<void(const char* bodyJunk, int junkLength)> bodyRead) {
    if (requestState != requeststates::BODY) {
        int i = 0;

        while (i < junkLen && requestState != ERROR && requestState != BODY) {
            const char& ch = junk[i++];
            if (ch != '\r') { // '\r' can be ignored completely as long as we are not receiving the body of the document
                switch (lineState) {
                case linestate::READ:
                    if (ch == '\n') {
                        if (headerLine.empty()) {
                            lineRead(headerLine);
                        } else {
                            lineState = linestate::EOL;
                        }
                    } else {
                        headerLine += ch;
                    }
                    break;
                case linestate::EOL:
                    if (ch == '\n') {
                        lineRead(headerLine);
                        headerLine.clear();
                        lineRead(headerLine);
                    } else if (!isblank(ch)) {
                        lineRead(headerLine);
                        headerLine.clear();
                        headerLine += ch;
                    } else {
                        headerLine += ch;
                    }
                    lineState = linestate::READ;
                    break;
                }
            }
        }
        if (i != junkLen) {
            bodyRead(junk + i, junkLen - i);
        }
    } else {
        bodyRead(junk, junkLen);
    }
}


void HTTPContext::parseRequestLine(const std::string& line) {
    std::pair<std::string, std::string> pair;

    pair = httputils::str_split(line, ' ');
    method = pair.first;
    httputils::to_lower(method);

    pair = httputils::str_split(pair.second, ' ');
    httpVersion = pair.second;

    /** Belongs into url-parser middleware */
    pair = httputils::str_split(httputils::url_decode(pair.first), '?');
    originalUrl = pair.first;
    path = httputils::str_split_last(pair.first, '/').first;

    if (path.empty()) {
        path = "/";
    }

    std::string queries = pair.second;

    while (!queries.empty()) {
        pair = httputils::str_split(queries, '&');
        queries = pair.second;
        pair = httputils::str_split(pair.first, '=');
        queryMap.insert(pair);
    }
}


void HTTPContext::requestReady() {
    webApp->dispatch(method, request, response);

    if (requestHeader.find("connection") != requestHeader.end()) {
        if (requestHeader.find("connection")->second == "Close") {
            connectedSocket->end();
        }
    } else {
        connectedSocket->end();
    }

    this->reset();
}


void HTTPContext::parseCookie(const std::string& value) {
    std::istringstream cookyStream(value);

    for (std::string cookie; std::getline(cookyStream, cookie, ';');) {
        std::pair<std::string, std::string> splitted = httputils::str_split(cookie, '=');

        httputils::str_trimm(splitted.first);
        httputils::str_trimm(splitted.second);

        requestCookies.insert(splitted);
    }
}


void HTTPContext::addRequestHeader(const std::string& line) {
    if (!line.empty()) {
        std::pair<std::string, std::string> splitted = httputils::str_split(line, ':');
        httputils::str_trimm(splitted.first);
        httputils::str_trimm(splitted.second);

        httputils::to_lower(splitted.first);

        if (!splitted.second.empty()) {
            if (splitted.first == "cookie") {
                parseCookie(splitted.second);
            } else {
                requestHeader.insert(splitted);
                if (splitted.first == "content-length") {
                    bodyLength = std::stoi(splitted.second);
                    bodyData = new char[bodyLength];
                }
            }
        }
    }
}


void HTTPContext::send(const char* puffer, int size) {
    responseHeader.insert({"Content-Type", "application/octet-stream"});
    responseHeader.insert({"Content-Length", std::to_string(size)});

    this->sendHeader();
    connectedSocket->send(puffer, size);
}


void HTTPContext::send(const std::string& puffer) {
    responseHeader.insert({"Content-Type", "text/html; charset=utf-8"});

    this->send(puffer.c_str(), puffer.size());
}


void HTTPContext::sendFile(const std::string& url, const std::function<void(int ret)>& onError) {
    std::string absolutFileName = webApp->getRootDir() + url;

    std::error_code ec;

    if (std::filesystem::exists(absolutFileName)) {
        absolutFileName = std::filesystem::canonical(absolutFileName);

        if (absolutFileName.rfind(webApp->getRootDir(), 0) == 0 && std::filesystem::is_regular_file(absolutFileName, ec) && !ec) {
            responseHeader.insert({"Content-Type", MimeTypes::contentType(absolutFileName)});
            responseHeader.insert_or_assign("Content-Length", std::to_string(std::filesystem::file_size(absolutFileName)));
            responseHeader.insert({"Last-Modified", httputils::file_mod_http_date(absolutFileName)});
            this->sendHeader();
            connectedSocket->sendFile(absolutFileName, onError);
        } else {
            this->responseStatus = 403;
            this->end();
            if (onError) {
                onError(EACCES);
            }
        }
    } else {
        this->responseStatus = 404;
        this->end();
        if (onError) {
            onError(ENOENT);
        }
    }
}


void HTTPContext::sendHeader() {
    connectedSocket->send("HTTP/1.1 " + std::to_string(responseStatus) + " " + HTTPStatusCode::reason(responseStatus) + "\r\n");
    connectedSocket->send("Date: " + httputils::to_http_date() + "\r\n");

    responseHeader.insert({"Cache-Control", "public, max-age=0"});
    responseHeader.insert({"Accept-Ranges", "bytes"});
    responseHeader.insert({"X-Powered-By", "snode.c"});

    std::for_each(responseHeader.begin(), responseHeader.end(),
                  [this](const std::pair<const std::string&, const std::string&>& header) -> void {
                      this->connectedSocket->send(header.first + ": " + header.second + "\r\n");
                  });

    std::for_each(responseCookies.begin(), responseCookies.end(),
                  [this](const std::pair<const std::string&, const ResponseCookie&>& cookie) -> void {
                      std::string cookieString = cookie.first + "=" + cookie.second.value;

                      std::for_each(cookie.second.options.begin(), cookie.second.options.end(),
                                    [&cookieString](const std::pair<const std::string&, const std::string&>& options) -> void {
                                        cookieString += "; " + options.first + ((options.second != "") ? "=" + options.second : "");
                                    });
                      this->connectedSocket->send("Set-Cookie: " + cookieString + "\r\n");
                  });

    connectedSocket->send("\r\n");

    headerSend = true;
}


void HTTPContext::end() {
    this->responseHeader.insert({"Content-Length", "0"});
    this->sendHeader();
}


void HTTPContext::reset() {
    this->responseStatus = 200;
    this->requestState = requeststates::REQUEST;
    this->lineState = linestate::READ;

    this->requestHeader.clear();
    this->method.clear();
    this->originalUrl.clear();
    this->httpVersion.clear();
    this->path.clear();
    this->queryMap.clear();

    this->responseHeader.clear();
    this->requestCookies.clear();
    this->responseCookies.clear();

    if (this->bodyData != 0) {
        delete this->bodyData;
        this->bodyData = 0;
    }
    this->bodyLength = 0;
    this->bodyPointer = 0;
    this->headerSend = false;
}
