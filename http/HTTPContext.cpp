#include "HTTPContext.h"

#include <string.h>

#include <filesystem>
#include <iostream>
#include <sstream>

#include "ConnectedSocket.h"
#include "HTTPServer.h"
#include "HTTPStatusCodes.h"
#include "MimeTypes.h"

HTTPContext::HTTPContext(HTTPServer* ss, ConnectedSocket* cs) : cs(cs), ss(ss), bodyData(0), bodyLength(0), state(states::REQUEST), bodyPointer(0), line(""), headerSent(false), responseStatus(200), linestate(READ) {}

void HTTPContext::httpRequest(std::string line) {
    if (state != BODY) {
        readLine(line, [&] (std::string line) -> void {
            switch (state) {
                case REQUEST:
                    if (line.empty()) {
                        this->responseStatus = 400;
                        this->responseHeader["Connection"] = "close";
                        cs->ConnectedSocket::end();
                        this->end();
                        state = ERROR;
                    } else {
                        requestLine = line;
                        state = HEADER;
                    }
                    break;
                case HEADER:
                    if (line.empty()) {
                        if (bodyLength != 0) {
                            state = BODY;
                        } else {
                            this->requestReady();
                            state = REQUEST;
                        }
                    } else {
                        this->addRequestHeader(line);
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

void HTTPContext::readLine(std::string readPuffer, std::function<void (std::string)> lineRead) {
    for(int i = 0; i < readPuffer.size() && state != ERROR; i++) {
        char& ch = readPuffer[i];
        
        if (ch != '\r') { // '\r' can be ignored completely
            switch(linestate) {
                case READ:
                    if (ch == '\n') {
                        if (hl.empty()) {
                            lineRead(hl);
                        } else {
                            linestate = EOL;
                        }
                    } else {
                        hl += ch;
                    }
                    break;
                case EOL:
                    if (ch == '\n') {
                        lineRead(hl);
                        hl.clear();
                        lineRead(hl);
                    } else if (!isblank(ch)) {
                        lineRead(hl);
                        hl.clear();
                        hl += ch;
                    } else {
                        hl += ch;
                    }
                    linestate = READ;
                    break;
            }
        }
    }
    readPuffer.clear();
}


void HTTPContext::requestReady() {
    Request req(this);
    Response res(this);
    ss->process(req, res);
}


void HTTPContext::addRequestHeader(std::string& line) {
    if (!line.empty()) {
        
        std::transform(line.begin(), line.end(), line.begin(), ::tolower);
        
        std::string key;
        std::string token;
        
        std::istringstream tokenStream(line);
        std::getline(tokenStream, key, ':');
        std::getline(tokenStream, token, '\n');
        
        int strBegin = token.find_first_not_of(" \t");
        int strEnd = token.find_last_not_of(" \t");
        int strRange = strEnd - strBegin + 1;
        
        if (strBegin != std::string::npos) {
            requestHeader[key] = token.substr(strBegin, strRange);
            
            if (key == "content-length") {
                bodyLength = std::stoi(requestHeader[key]);
                bodyData = new char[bodyLength];
            }
        }
    }
}


void HTTPContext::sendJunk(const char* puffer, int size) {
    this->sendHeader();
    cs->send(puffer, size);
}


void HTTPContext::send(const char* puffer, int size) {
    if (responseHeader.find("Content-Type") == responseHeader.end()) {
        responseHeader["Content-Type"] = "application/octet-stream";
    }
    responseHeader["Content-Length"] = size;
    this->sendHeader();
    cs->send(puffer, size);
    this->end();
}


void HTTPContext::send(const std::string& puffer) {
    if (responseHeader.find("Content-Type") == responseHeader.end()) {
        responseHeader["Content-Type"] = "text/html; charset=utf-8";
    }
    responseHeader["Content-Length"] = puffer.size();
    this->sendHeader();
    cs->send(puffer);
    this->end();
}


void HTTPContext::sendFile(const std::string& file) {
    
    std::string absolutFileName = ss->getRootDir() + file;
    
    if (std::filesystem::exists(absolutFileName)) {
        if (responseHeader.find("Content-Type") == responseHeader.end()) {
            responseHeader["Content-Type"] = MimeTypes::contentType(absolutFileName);
        }
        responseHeader["Content-Length"] = std::to_string(std::filesystem::file_size(absolutFileName));
        this->sendHeader();
        cs->sendFile(absolutFileName);
        this->end();
    } else {
        this->responseStatus = 404;
        this->responseHeader["Connection"] = "close";
        cs->end();
        this->end();
    }
}


void HTTPContext::sendHeader() {
    if (!this->headerSent) {
        cs->send("HTTP/1.1 " + std::to_string( responseStatus ) + " " + HTTPStatusCode::reason( responseStatus )+ "\r\n");
        
        for (std::map<std::string, std::string>::iterator it = responseHeader.begin(); it != responseHeader.end(); ++it) {
            cs->send((*it).first + ": " + (*it).second + "\r\n");
        }
        cs->send("\r\n");
        this->headerSent = true;
    }
}

void HTTPContext::end() {
    if (this->state != REQUEST) {
        this->sendHeader();
        this->headerSent = false;
        this->responseHeader.clear();
        this->responseStatus = 200;
        this->state = REQUEST;
        
        if (this->requestHeader["connection"] == "close") {
            cs->end();
        }

        this->requestHeader.clear();
        this->requestLine.clear();

        if (this->bodyData != 0) {
            delete this->bodyData;
            this->bodyData = 0;
            this->bodyLength = 0;
            this->bodyPointer = 0;
        }
    }
}

