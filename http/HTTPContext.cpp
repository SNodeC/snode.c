#include "HTTPContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string.h>

#include <filesystem>
#include <sstream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ConnectedSocket.h"
#include "HTTPServer.h"
#include "HTTPStatusCodes.h"
#include "MimeTypes.h"

#include "Request.h"
#include "Response.h"

#include "httputils.h"


HTTPContext::HTTPContext(HTTPServer* serverSocket, ConnectedSocket* connectedSocket)
: connectedSocket(connectedSocket), serverSocket(serverSocket), bodyData(0) {
    this->reset();
}


void HTTPContext::parseHttpRequest(const char* junk, ssize_t n) {
    if (requestState != BODY) {
        readLine(junk, n, [&] (const std::string& line) -> void {
            switch (requestState) {
            case REQUEST:
                if (!line.empty()) {
                    parseRequestLine(line);
                    requestState = HEADER;
                } else {
                    this->responseStatus = 400;
                    this->responseHeader.insert({"Connection", "close"});
                    connectedSocket->end();
                    this->end();
                    requestState = ERROR;
                }
                break;
            case HEADER:
                if (!line.empty()) {
                    this->addRequestHeader(line);
                } else {
                    if (bodyLength != 0) {
                        requestState = BODY;
                    } else {
                        this->requestReady();
                        requestState = REQUEST;
                    }
                }
                break;
            case BODY:
            case ERROR:
                break;
            }
        });
    } else {
        int n = line.size();

        if (bodyLength - bodyPointer < n) {
            n = bodyLength - bodyPointer;
        }

        memcpy(bodyData + bodyPointer, line.c_str(), n);
        bodyPointer += n;

        if (bodyPointer == bodyLength)  {
            this->requestReady();
            requestState = REQUEST;
        }
    }
}


void HTTPContext::readLine(const char* junk, ssize_t n, const std::function<void (std::string&)>& lineRead) {
    for(int i = 0; i < n && requestState != ERROR; i++) {
        const char& ch = junk[i];

        if (ch != '\r') { // '\r' can be ignored completely
            switch(linestate) {
            case READ:
                if (ch == '\n') {
                    if (headerLine.empty()) {
                        lineRead(headerLine);
                    } else {
                        linestate = EOL;
                    }
                } else {
                    headerLine += ch;
                }
                break;
            case EOL:
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
                linestate = READ;
                break;
            }
        }
    }
}


void HTTPContext::parseRequestLine(const std::string& line) {
    std::istringstream requestLineStream(httputils::url_decode(line));
    
    std::getline(requestLineStream, method, ' ');
    std::getline(requestLineStream, requestUri, ' ');
    std::getline(requestLineStream, httpVersion, '\r');
    
    std::string queries;
    std::istringstream requestUriStream(requestUri);
    std::getline(requestUriStream, path, '?');
    std::getline(requestUriStream, queries, '#');
    std::getline(requestUriStream, fragment);
    
    std::istringstream queriesStream(queries);
    for (std::string query; std::getline(queriesStream, query, '&'); ) {
        std::pair<std::string, std::string> splitted = httputils::str_split(query, '=');
        queryMap.insert(splitted);
    }
}


void HTTPContext::requestReady() {
    serverSocket->process(Request(this), Response(this));
}


void HTTPContext::parseCookie(const std::string& value) {
    std::istringstream cookyStream(value);
    
    for (std::string cookie; std::getline(cookyStream, cookie, ';'); ) {
        std::pair<std::string, std::string> splitted = httputils::str_split(cookie, '=');
        
        httputils::str_trimm(splitted.first);
        httputils::str_trimm(splitted.second);
        
        requestCookies.insert(splitted);
    }
}


void HTTPContext::addRequestHeader(const std::string& line) {
    if (!line.empty()) {
        std::pair<std::string, std::string> splitted = httputils::str_split(line, ':', '\r');
        httputils::str_trimm(splitted.first);
        httputils::str_trimm(splitted.second);
        
        std::transform(splitted.first.begin(), splitted.first.end(), splitted.first.begin(), ::tolower);

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

/*
void HTTPContext::sendJunk(const char* puffer, int size) {
    this->sendHeader();
    connectedSocket->send(puffer, size);
}
*/

void HTTPContext::send(const char* puffer, int size) {
    if (responseHeader.find("Content-Type") == responseHeader.end()) {
        responseHeader.insert({"Content-Type", "application/octet-stream"});
    }
    responseHeader.insert({"Content-Length", std::to_string(size)});
    this->sendHeader();
    connectedSocket->send(puffer, size);
}


void HTTPContext::send(const std::string& puffer) {
    if (responseHeader.find("Content-Type") == responseHeader.end()) {
        responseHeader.insert({"Content-Type", "text/html; charset=utf-8"});
    }
    responseHeader.insert({"Content-Length", std::to_string(puffer.size())});
    this->sendHeader();
    connectedSocket->send(puffer);
}


void HTTPContext::sendFile(const std::string& url, const std::function<void (int ret)>& onError) {
    std::string absolutFileName = serverSocket->getRootDir() + url;

    if (std::filesystem::exists(absolutFileName)) {
        if (responseHeader.find("Content-Type") == responseHeader.end()) {
            responseHeader.insert({"Content-Type", MimeTypes::contentType(absolutFileName)});
        }
        responseHeader.insert({"Content-Length", std::to_string(std::filesystem::file_size(absolutFileName))});
        this->sendHeader();
        connectedSocket->sendFile(absolutFileName, onError);
    } else {
        this->responseStatus = 404;
        this->responseHeader.insert({"Connection", "close"});
        this->sendHeader();
        onError(ENOENT);
    }
}


void HTTPContext::sendHeader() {
    connectedSocket->send("HTTP/1.1 " + std::to_string( responseStatus ) + " " + HTTPStatusCode::reason( responseStatus ) +  "\r\n");
    connectedSocket->send("Date: " + httputils::to_http_date() + "\r\n");
    
    for (std::multimap<std::string, std::string>::iterator it = responseHeader.begin(); it != responseHeader.end(); ++it) {
        connectedSocket->send(it->first + ": " + it->second + "\r\n");
    }
    
    
    for (std::multimap<std::string, std::string>::iterator it = responseCookies.begin(); it != responseCookies.end(); ++it) {
        connectedSocket->send("Set-Cookie: " + it->first + "=" + it->second + "\r\n");
    }
        
    connectedSocket->send("\r\n");
}


void HTTPContext::end() {
    this->sendHeader();
}


void HTTPContext::reset() {  
    if (requestHeader.find("connection")->second == "close") {
        connectedSocket->end();
    }
    
    this->responseHeader.clear();
    this->responseStatus = 200;
    this->requestState = REQUEST;
    this->linestate = READ;
    this->line.clear();
    
    this->requestHeader.clear();
    this->method.clear();
    this->requestUri.clear();
    this->httpVersion.clear();
    this->path.clear();
    this->fragment.clear();
    this->queryMap.clear();
    
    if (this->bodyData != 0) {
        delete this->bodyData;
        this->bodyData = 0;
    }
    this->bodyLength = 0;
    this->bodyPointer = 0;
}
