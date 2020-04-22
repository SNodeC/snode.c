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


AcceptedSocket::AcceptedSocket(int csFd, Server* ss) : ConnectedSocket(csFd, ss), request(this), response(this), bodyData(0), bodyLength(0), state(states::REQUEST), bodyPointer(0), line(""), headerSent(false), responseStatus(200) {
}


AcceptedSocket::~AcceptedSocket() {
    if (bodyData != 0) {
        delete bodyData;
    }
}


void AcceptedSocket::requestReady() {
    serverSocket->process(request, response);
    state = HEADER;
    if (this->bodyData != 0) {
        if (requestHeader["connection"] == "close") {
            std::cout << "AEOF" << std::endl;
            this->ConnectedSocket::end();
        }
        requestHeader.clear();
        requestLine.clear();
        
        bodyData = 0;
        bodyLength = 0;
        bodyPointer = 0;
        delete bodyData;
    }
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
                bodyData = new char[bodyLength];
            }
        }
    }
}


void AcceptedSocket::readEvent() {
    ConnectedSocket::readEvent();

    if (state != BODY) {
        for (size_t i = 0; i < readBuffer.size() && state != BODY; i++) {
            char& ch = readBuffer[i];
            switch(state) {
            case REQUEST:
                if (ch != '\r' && ch != '\n') {
                    line += ch;
                } else if (ch == '\n') {
                    state = REQUEST_LB;
                }
                break;
            case REQUEST_LB:
                if (ch != '\r') {
                    if (ch == '\n') {
                        line.clear();
                        if (bodyLength > 0) {
                            readBuffer.erase(0, i + 1);
                            state = BODY;
                        } else {
                            this->requestReady();
                        }
                    } else if (isspace(ch)) {
                        line += ch;
                        state = REQUEST;
                    } else {
                        requestLine = line;
                        line.clear();
                        line += ch;
                        state = HEADER;
                    }
                }
                break;
            case HEADER:
                if (ch != '\r') {
                    if (ch != '\n') {
                        line += ch;
                    } else {
                        state = HEADER_LB;
                    }
                }
                break;
            case HEADER_LB:
                if (ch != '\r') {
                    if (ch == '\n' || !isspace(ch)) {
                        this->addRequestHeader(line);
                        line.clear();
                        if (ch != '\n') {
                            line += ch;
                            state = HEADER;
                        } else {
                            if (bodyLength > 0) {
                                readBuffer.erase(0, i + 1);
                                state = BODY;
                            } else {
                                this->requestReady();
                            }
                        }
                    } else {
                        line += ch;
                        state = HEADER;
                    }
                }
                break;
            case BODY:
                std::cerr << "Body: This should not happen" << ch << std::endl;
                break;
            }
        }
    }
    
    if (state == BODY) {
        int n = readBuffer.size();
        
        if (bodyLength - bodyPointer < n) {
            n = bodyLength - bodyPointer;
        }
        
        memcpy(bodyData + bodyPointer, readBuffer.c_str(), n);
        bodyPointer += n;
        
        if (bodyPointer == bodyLength)  {
            bodyData[bodyLength] = 0;
            this->requestReady();
            state = REQUEST;
        }
        
    }
    
    readBuffer.clear();
}


void AcceptedSocket::writeEvent() {
    ConnectedSocket::writeEvent();

    if (writeBuffer.empty()) {
        state = states::REQUEST;
    }
}


void AcceptedSocket::send(const char* buffer, int size) {
    if (responseHeader.find("Content-Type") == responseHeader.end()) {
        responseHeader["Content-Type"] = "application/octet-stream";
    }
    responseHeader["Content-Length"] = size;
    this->sendHeader();
    ConnectedSocket::send(buffer, size);
    this->end();
}


void AcceptedSocket::send(const std::string& junk) {
    if (responseHeader.find("Content-Type") == responseHeader.end()) {
        responseHeader["Content-Type"] = "text/html; charset=utf-8";
    }
    responseHeader["Content-Length"] = junk.size();
    this->sendHeader();
    ConnectedSocket::send(junk);
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
    this->sendHeader();
/*
    if (requestHeader["Connection"] == "close" || force) {
        std::cout << "AEOF" << std::endl;
        this->ConnectedSocket::end();
    }
*/
    this->headerSent = false;
    this->responseHeader.clear();
    this->responseStatus = 200;
}
