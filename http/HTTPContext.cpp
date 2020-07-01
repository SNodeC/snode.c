#include "HTTPContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <numeric>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPStatusCodes.h"
#include "MimeTypes.h"
#include "WebApp.h"
#include "file/FileReader.h"
#include "httputils.h"
#include "socket/SocketConnection.h"


HTTPContext::HTTPContext(const WebApp& webApp, SocketConnection* connectedSocket)
    : connectedSocket(connectedSocket)
    , webApp(webApp)
    , request(this)
    , response(this) {
    this->prepareForRequest();
}


void HTTPContext::stopFileReader() {
    if (fileReader != nullptr) {
        fileReader->stop();
    }
}


void HTTPContext::onReadError(int errnum) {
    stopFileReader();

    if (errnum != 0 && errnum != ECONNRESET) {
        perror("HTTPContext");
    }
}


void HTTPContext::onWriteError(int errnum) {
    stopFileReader();

    if (errnum != 0 && errnum != ECONNRESET) {
        perror("HTTPContext");
    }
}


void HTTPContext::receiveRequest(const char* junk, ssize_t junkLen) {
    if (requestInProgress) {
        this->connectedSocket->end();
    } else {
        parseRequest(
            junk, junkLen,
            [&](const std::string& line) -> void { // header data
                switch (requestState) {
                case requeststates::REQUEST:
                    if (!line.empty()) {
                        parseRequestLine(line);
                        requestState = requeststates::HEADER;
                    } else {
                        response.responseStatus = 400;
                        response.responseHeader.insert({"Connection", "Close"});
                        this->end();
                        connectedSocket->end();
                        requestState = requeststates::ERROR;
                    }
                    break;
                case requeststates::HEADER:
                    if (!line.empty()) {
                        this->addRequestLine(line);
                    } else {
                        headerRead();
                    }
                    break;
                case requeststates::BODY:
                case requeststates::ERROR:
                    break;
                }
            },
            [&](const char* bodyJunk, int junkLen) -> void { // body data
                if (request.bodyLength - bodyPointer < junkLen) {
                    junkLen = request.bodyLength - bodyPointer;
                }
                memcpy(request.body + bodyPointer, bodyJunk, junkLen);
                bodyPointer += junkLen;
                if (bodyPointer == request.bodyLength) {
                    this->bodyRead();
                }
            });
    }
}


void HTTPContext::parseRequest(const char* junk, ssize_t junkLen, const std::function<void(std::string&)>& lineRead,
                               const std::function<void(const char* bodyJunk, int junkLength)>& readBody) {
    if (requestState != requeststates::BODY) {
        int n = 0;

        while (n < junkLen && requestState != ERROR && requestState != BODY) {
            const char& ch = junk[n++];
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
        if (n != junkLen) {
            readBody(junk + n, junkLen - n);
        }
    } else {
        readBody(junk, junkLen);
    }
}


void HTTPContext::parseRequestLine(const std::string& line) {
    std::pair<std::string, std::string> pair;

    pair = httputils::str_split(line, ' ');
    request.method = pair.first;
    httputils::to_lower(request.method);

    pair = httputils::str_split(pair.second, ' ');
    request.httpVersion = pair.second;

    /** Belongs into url-parser middleware */
    pair = httputils::str_split(httputils::url_decode(pair.first), '?');
    request.originalUrl = pair.first;
    request.path = httputils::str_split_last(pair.first, '/').first;

    if (request.path.empty()) {
        request.path = "/";
    }

    std::string queries = pair.second;

    while (!queries.empty()) {
        pair = httputils::str_split(queries, '&');
        queries = pair.second;
        pair = httputils::str_split(pair.first, '=');
        request.queryMap.insert(pair);
    }
}


void HTTPContext::addRequestLine(const std::string& line) {
    if (!line.empty()) {
        std::pair<std::string, std::string> splitted = httputils::str_split(line, ':');
        httputils::str_trimm(splitted.first);
        httputils::str_trimm(splitted.second);

        httputils::to_lower(splitted.first);

        if (!splitted.second.empty()) {
            if (splitted.first == "cookie") {
                parseCookie(splitted.second);
            } else {
                request.requestHeader.insert(splitted);
            }
        }
    }
}


void HTTPContext::parseCookie(const std::string& value) {
    std::istringstream cookyStream(value);

    for (std::string cookie; std::getline(cookyStream, cookie, ';');) {
        std::pair<std::string, std::string> splitted = httputils::str_split(cookie, '=');

        httputils::str_trimm(splitted.first);
        httputils::str_trimm(splitted.second);

        request.requestCookies.insert(splitted);
    }
}


void HTTPContext::headerRead() {
    if (request.header("content-length") != "") {
        request.bodyLength = std::stoi(request.header("content-length"));
    }

    if (request.bodyLength > 0) {
        request.body = new char[request.bodyLength];
        requestState = requeststates::BODY;
    } else {
        this->requestReady();
        requestState = requeststates::REQUEST;
    }
}


void HTTPContext::bodyRead() {
    this->requestReady();
}


void HTTPContext::requestReady() {
    this->requestInProgress = true;

    webApp.dispatch(request, response);

    if (request.requestHeader.find("connection") != request.requestHeader.end()) {
        if (request.requestHeader.find("connection")->second == "Close") {
            connectedSocket->end();
        }
    } else {
        connectedSocket->end();
    }
}


void HTTPContext::enqueue(const char* buf, size_t len) {
    connectedSocket->enqueue(buf, len);

    if (headerSend) {
        sendLen += len;
        if (sendLen == contentLength) {
            prepareForRequest();
        }
    }
}


void HTTPContext::enqueue(const std::string& str) {
    this->enqueue(str.c_str(), str.size());
}


void HTTPContext::send(const char* buffer, int size) {
    response.responseHeader.insert({"Content-Type", "application/octet-stream"});
    response.responseHeader.insert({"Content-Length", std::to_string(size)});

    this->sendHeader();
    this->enqueue(buffer, size);
}


void HTTPContext::send(const std::string& buffer) {
    response.responseHeader.insert({"Content-Type", "text/html; charset=utf-8"});

    this->send(buffer.c_str(), buffer.size());
}


void HTTPContext::sendFile(const std::string& file, const std::function<void(int ret)>& onError) {
    std::string absolutFileName = webApp.getRootDir() + file;

    if (std::filesystem::exists(absolutFileName)) {
        std::error_code ec;
        absolutFileName = std::filesystem::canonical(absolutFileName);

        if (absolutFileName.rfind(webApp.getRootDir(), 0) == 0 && std::filesystem::is_regular_file(absolutFileName, ec) && !ec) {
            response.responseHeader.insert({"Content-Type", MimeTypes::contentType(absolutFileName)});
            response.responseHeader.insert_or_assign("Content-Length", std::to_string(std::filesystem::file_size(absolutFileName)));
            response.responseHeader.insert({"Last-Modified", httputils::file_mod_http_date(absolutFileName)});
            this->sendHeader();

            fileReader = FileReader::read(
                absolutFileName,
                [this](char* data, int length) -> void {
                    this->enqueue(data, length);
                },
                [this, onError](int err) -> void {
                    if (onError) {
                        onError(err);
                    }
                    if (err != 0) {
                        connectedSocket->end();
                    }
                });
        } else {
            response.responseStatus = 403;
            errno = EACCES;
            this->end();
            if (onError) {
                onError(EACCES);
            }
        }
    } else {
        response.responseStatus = 404;
        errno = ENOENT;
        this->end();
        if (onError) {
            onError(ENOENT);
        }
    }
}


void HTTPContext::sendHeader() {
    if (!headerSend) {
        this->enqueue("HTTP/1.1 " + std::to_string(response.responseStatus) + " " + HTTPStatusCode::reason(response.responseStatus) +
                      "\r\n");
        this->enqueue("Date: " + httputils::to_http_date() + "\r\n");

        response.responseHeader.insert({"Cache-Control", "public, max-age=0"});
        response.responseHeader.insert({"Accept-Ranges", "bytes"});
        response.responseHeader.insert({"X-Powered-By", "snode.c"});

        for (const std::pair<const std::string, std::string>& header : response.responseHeader) {
            this->enqueue(header.first + ": " + header.second + "\r\n");
        }

        for (const std::pair<const std::string, Response::ResponseCookie>& cookie : response.responseCookies) {
            std::string cookieString =
                std::accumulate(cookie.second.options.begin(), cookie.second.options.end(), cookie.first + "=" + cookie.second.value,
                                [](const std::string& str, const std::pair<const std::string&, const std::string&> option) -> std::string {
                                    return str + "; " + option.first + (!option.second.empty() ? "=" + option.second : "");
                                });
            this->enqueue("Set-Cookie: " + cookieString + "\r\n");
        }

        this->enqueue("\r\n");

        headerSend = true;

        contentLength = std::stoi(response.responseHeader.find("Content-Length")->second);
    }
}


void HTTPContext::end() {
    response.responseHeader.insert({"Content-Length", "0"});
    this->sendHeader();
    this->prepareForRequest();
}


void HTTPContext::prepareForRequest() {
    this->requestState = requeststates::REQUEST;
    this->lineState = linestate::READ;
    this->bodyPointer = 0;
    this->headerSend = false;
    this->sendLen = 0;
    this->stopFileReader();
    this->contentLength = 0;
    this->fileReader = nullptr;
    this->requestInProgress = false;

    request.reset();
    response.reset();
}
