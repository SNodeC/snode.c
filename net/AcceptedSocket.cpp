#include "AcceptedSocket.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <sstream>

#include <string.h>

#include "MimeTypes.h"
#include "ServerSocket.h"
#include "SocketMultiplexer.h"
#include "HTTPStatusCodes.h"


AcceptedSocket::AcceptedSocket(int csFd, Server* ss) : ConnectedSocket(csFd, ss), request(this), response(this), bodyData(0), bodyLength(0), state(states::REQUEST), bodyPointer(0), line(""), headerSent(false), responseStatus(200), linestate(READ) {
}


AcceptedSocket::~AcceptedSocket() {
    if (bodyData != 0) {
        delete bodyData;
    }
}


void AcceptedSocket::requestReady() {
    serverSocket->process(request, response);
}


void AcceptedSocket::addRequestHeader(std::string& line) {
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
                bodyData = new char[bodyLength + 1];
            }
        }
    }
}


void AcceptedSocket::readLine(std::function<void (std::string)> lineRead) {
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


void AcceptedSocket::readEvent() {
    ConnectedSocket::readEvent();
    
    if (state != BODY) {
        readLine([&] (std::string line) -> void {
            std::cout << "Line: " << line << std::endl;
    
            switch (state) {
                case REQUEST:
                    if (line.empty()) {
                        this->responseStatus = 400;
                        this->responseHeader["Connection"] = "close";
                        this->ConnectedSocket::end();
                        this->end();
                        std::cout << "AEOF" << std::endl;
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
                            std::cout << "Request ready" << std::endl;
                            this->requestReady();
                            state = REQUEST;
                        }
                    } else {
                        this->addRequestHeader(line);
                    }
                    break;
                case BODY:
                case ERROR:
                    std::cout << "State: BODY or ERROR. This should not happen" << std::endl;
            }
        });
    } else {
        int n = readPuffer.size();
        
        if (bodyLength - bodyPointer < n) {
            n = bodyLength - bodyPointer;
        }
        
        memcpy(bodyData + bodyPointer, readPuffer.c_str(), n);
        bodyPointer += n;
        
        if (bodyPointer == bodyLength)  {
            bodyData[bodyLength] = 0;
            std::cout << "Body: " << bodyData << std::endl;
            this->requestReady();
            state = REQUEST;
        }
        readPuffer.clear();
    }
}


void AcceptedSocket::writeEvent() {
    ConnectedSocket::writeEvent();

    if (writePuffer.empty()) {
        state = REQUEST;
    }
}


void AcceptedSocket::sendJunk(const char* puffer, int size) {
    this->sendHeader();
    ConnectedSocket::send(puffer, size);
}


void AcceptedSocket::send(const char* puffer, int size) {
    if (responseHeader.find("Content-Type") == responseHeader.end()) {
        responseHeader["Content-Type"] = "application/octet-stream";
    }
    responseHeader["Content-Length"] = size;
    this->sendHeader();
    ConnectedSocket::send(puffer, size);
    this->end();
}


void AcceptedSocket::send(const std::string& puffer) {
    if (responseHeader.find("Content-Type") == responseHeader.end()) {
        responseHeader["Content-Type"] = "text/html; charset=utf-8";
    }
    responseHeader["Content-Length"] = puffer.size();
    this->sendHeader();
    ConnectedSocket::send(puffer);
    this->end();
}


void AcceptedSocket::sendFile(const std::string& file) {

    std::string absolutFileName = serverSocket->getRootDir() + file;

    if (std::filesystem::exists(absolutFileName)) {
        if (responseHeader.find("Content-Type") == responseHeader.end()) {
            responseHeader["Content-Type"] = MimeTypes::contentType(absolutFileName);
        }
        responseHeader["Content-Length"] = std::to_string(std::filesystem::file_size(absolutFileName));
        this->sendHeader();
        ConnectedSocket::sendFile(absolutFileName);
        this->end();
    } else {
        this->responseStatus = 404;
        this->responseHeader["Connection"] = "close";
        this->ConnectedSocket::end();
        this->end();
        std::cout << "AEOF" << std::endl;
    }
}


void AcceptedSocket::sendHeader() {
    if (!this->headerSent) {
        this->ConnectedSocket::send("HTTP/1.1 " + std::to_string( responseStatus ) + " " + HTTPStatusCode::reason( responseStatus )+ "\r\n");

        for (std::map<std::string, std::string>::iterator it = responseHeader.begin(); it != responseHeader.end(); ++it) {
            this->ConnectedSocket::send((*it).first + ": " + (*it).second + "\r\n");
        }
        this->ConnectedSocket::send("\r\n");
        this->headerSent = true;
    }
}


void AcceptedSocket::end() {
    if (this->state != REQUEST) {
        this->sendHeader();
        this->headerSent = false;
        this->responseHeader.clear();
        this->responseStatus = 200;
        this->state = REQUEST;
    
        if (this->requestHeader["connection"] == "close") {
            std::cout << "AEOF" << std::endl;
            this->ConnectedSocket::end();
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
