#include "HTTPContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string.h>

#include <algorithm>
#include <filesystem>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ConnectedSocket.h"
#include "HTTPServer.h"
#include "HTTPStatusCodes.h"
#include "MimeTypes.h"

#include "httputils.h"

#include <iostream>


HTTPContext::HTTPContext(HTTPServer* httpServer, ConnectedSocket* connectedSocket)
: connectedSocket(connectedSocket), httpServer(httpServer), bodyData(0), request(this), response(this) {
    this->reset();
}


void HTTPContext::receiveRequest(const char* junk, ssize_t n) {
    parseRequest(junk, n, 
        [&] (const std::string& line) -> void { // header data
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
        },
        [&] (const char* bodyJunk, int junkLen) -> void { // body data
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


void HTTPContext::parseRequest(const char* junk, ssize_t n, const std::function<void (std::string&)>& lineRead, const std::function<void (const char* bodyJunk, int junkLength)> bodyRead) {
    if (requestState != BODY) {
        int i = 0;
        
        while(i < n && requestState != ERROR && requestState != BODY) {
            const char& ch = junk[i++];

            if (ch != '\r') { // '\r' can be ignored completely as long as we are not receiving the body of the document
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
        if (i != n) {
            bodyRead(junk + i, n - i);
        }
    } else {
        bodyRead(junk, n);
    }
}


void HTTPContext::parseRequestLine(const std::string& line) {
    std::pair<std::string, std::string> pair;
    
    pair = httputils::str_split(line, ' ');
    method = pair.first;
    httputils::to_lower(method);
    
    pair = httputils::str_split(pair.second, ' ');
    originalUrl = httputils::url_decode(pair.first);
    httpVersion = pair.second;
    
    /** Belongs into url-parser middleware */
    pair = httputils::str_split(originalUrl, '?');
    originalUrl = pair.first;
    path = httputils::str_split_last(pair.first, '/').first;
    
    if (path.empty()) {
        path = "/";
    }
    
    std::string queries = pair.second;
    
    while(!queries.empty()) {
        pair = httputils::str_split(queries, '&');
        queries = pair.second;
        pair = httputils::str_split(pair.first, '=');
        queryMap.insert(pair);
    }
}


void HTTPContext::requestReady() {
    httpServer->dispatch(method, "", request, response);
    this->reset();
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
    this->send(puffer.c_str(), puffer.size());
}


void HTTPContext::sendFile(const std::string& url, const std::function<void (int ret)>& onError) {
    std::string absolutFileName = httpServer->getRootDir() + url;
    
    std::error_code ec;

    if (std::filesystem::exists(absolutFileName)) {
        absolutFileName = std::filesystem::canonical(absolutFileName);
        
        if (absolutFileName.rfind(httpServer->getRootDir(), 0) == 0) {
            if (responseHeader.find("Content-Type") == responseHeader.end()) {
                responseHeader.insert({"Content-Type", MimeTypes::contentType(absolutFileName)});
            }
            responseHeader.insert({"Content-Length", std::to_string(std::filesystem::file_size(absolutFileName))});
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
    connectedSocket->send("HTTP/1.1 " + std::to_string(responseStatus) + " " + HTTPStatusCode::reason(responseStatus) +  "\r\n");
    connectedSocket->send("Date: " + httputils::to_http_date() + "\r\n");
    
    for (std::multimap<std::string, std::string>::iterator it = responseHeader.begin(); it != responseHeader.end(); ++it) {
        connectedSocket->send(it->first + ": " + it->second + "\r\n");
    }
    
    for (std::map<std::string, std::string>::iterator it = responseCookies.begin(); it != responseCookies.end(); ++it) {
        connectedSocket->send("Set-Cookie: " + it->first + "=" + it->second + "\r\n");
    }
        
    connectedSocket->send("\r\n");
}


void HTTPContext::end() {
    this->responseHeader.insert({"Content-Length", std::to_string(0)});
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
    
    this->requestHeader.clear();
    this->method.clear();
    this->originalUrl.clear();
    this->httpVersion.clear();
    this->path.clear();
    this->queryMap.clear();
    
    if (this->bodyData != 0) {
        delete this->bodyData;
        this->bodyData = 0;
    }
    this->bodyLength = 0;
    this->bodyPointer = 0;
}
    
