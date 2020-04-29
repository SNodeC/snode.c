#include "HTTPContext.h"

#include <string.h>

#include <filesystem>
#include <sstream>

#include "ConnectedSocket.h"
#include "HTTPServer.h"
#include "HTTPStatusCodes.h"
#include "MimeTypes.h"


HTTPContext::HTTPContext(HTTPServer* serverSocket, ConnectedSocket* connectedSocket)
: connectedSocket(connectedSocket), serverSocket(serverSocket), bodyData(0), bodyLength(0), state(states::REQUEST), bodyPointer(0), line(""), responseStatus(200), linestate(READ) {}


void HTTPContext::parseHttpRequest(std::string line) {
//    std::cout << line << std::endl;
    if (state != BODY) {
        readLine(line, [&] (std::string& line) -> void {
            switch (state) {
            case REQUEST:
                if (!line.empty()) {
                    parseRequestLine(line);
                    state = HEADER;
                } else {
                    this->responseStatus = 400;
                    this->responseHeader.insert({"Connection", "close"});
                    connectedSocket->end();
                    this->end();
                    state = ERROR;
                }
                break;
            case HEADER:
                if (!line.empty()) {
                    this->addRequestHeader(line);
                } else {
                    if (bodyLength != 0) {
                        state = BODY;
                    } else {
                        this->requestReady();
                        state = REQUEST;
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
            state = REQUEST;
        }
    }
}


void HTTPContext::readLine(std::string readPuffer, std::function<void (std::string&)> lineRead) {
    for(int i = 0; i < readPuffer.size() && state != ERROR; i++) {
        char& ch = readPuffer[i];

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


void HTTPContext::parseRequestLine(std::string line) {
    std::istringstream requestLineStream(line);
    
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
        int equalSign = query.find_first_of('=');
        std::string key = query.substr(0, equalSign);
        std::string value = query.substr(equalSign + 1);
        
        queryMap[key] = value;
    }
}


void HTTPContext::requestReady() {
    serverSocket->process(Request(this), Response(this));
}


void HTTPContext::parseCookie(std::string value) {
    std::istringstream cookyStream(value);
    
    for (std::string cooky; std::getline(cookyStream, cooky, ';'); ) {
        int equalSign = cooky.find_first_of('=');
        
        std::string cookyName = cooky.substr(0, equalSign);
        cookyName.erase(cookyName.find_last_not_of(" \t") + 1);
        cookyName.erase(0, cookyName.find_first_not_of(" \t"));
        
        std::string cookyValue = cooky.substr(equalSign + 1);
        cookyValue.erase(cookyValue.find_last_not_of(" \t") + 1);
        cookyValue.erase(0, cookyValue.find_first_not_of(" \t"));
        
        requestCookies[cookyName] = cookyValue;
    }
}


void HTTPContext::addRequestHeader(std::string& line) {
    if (!line.empty()) {

        std::string key;
        std::string token;

        std::istringstream tokenStream(line);
        std::getline(tokenStream, key, ':');
        std::getline(tokenStream, token, '\r');
        
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        int strBegin = token.find_first_not_of(" \t");
        int strEnd = token.find_last_not_of(" \t");
        int strRange = strEnd - strBegin + 1;

        if (strBegin != std::string::npos) {
            if (key == "cookie") {
                parseCookie(token.substr(strBegin, strRange));
            } else {
                requestHeader.insert({key, token.substr(strBegin, strRange)});

                if (key == "content-length") {
                    bodyLength = std::stoi((*(requestHeader.find(key))).second);
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


void HTTPContext::sendFile(const std::string& url) {

    int strEnd = url.find_first_of("?");
    
    std::string file = url;
    if (strEnd != std::string::npos) {
        file = url.substr(0, strEnd);
    }
    
    std::string absolutFileName = serverSocket->getRootDir() + file;

    if (std::filesystem::exists(absolutFileName)) {
        if (responseHeader.find("Content-Type") == responseHeader.end()) {
            responseHeader.insert({"Content-Type", MimeTypes::contentType(absolutFileName)});
        }
        responseHeader.insert({"Content-Length", std::to_string(std::filesystem::file_size(absolutFileName))});
        this->sendHeader();
        connectedSocket->sendFile(absolutFileName);
    } else {
        this->responseStatus = 404;
        this->responseHeader.insert({"Connection", "close"});
        this->sendHeader();
        connectedSocket->end();
    }
}


void HTTPContext::sendHeader() {
    connectedSocket->send("HTTP/1.1 " + std::to_string( responseStatus ) + " " + HTTPStatusCode::reason( responseStatus ) +  "\r\n");

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
    this->state = REQUEST;
    
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
        this->bodyLength = 0;
        this->bodyPointer = 0;
    }
}
