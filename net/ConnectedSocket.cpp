#include <iostream>
#include <sstream>
#include <algorithm>
#include <string.h>

#include "ConnectedSocket.h"
#include "SocketMultiplexer.h"
#include "ServerSocket.h"

ConnectedSocket::ConnectedSocket(int csFd) : state(states::REQUEST), bodyData(0), bodyLength(0), bodyPointer(0), line("") {
    this->setFd(csFd);
}


ConnectedSocket::~ConnectedSocket() {
    if (bodyData != 0) {
        delete bodyData;
    }
}


InetAddress& ConnectedSocket::getRemoteAddress() {
    return remoteAddress;
}


void ConnectedSocket::setRemoteAddress(const InetAddress& remoteAddress) {
    this->remoteAddress = remoteAddress;
}

void ConnectedSocket::write(const char* buffer, int size) {
    writeBuffer.append(buffer, size);
    SocketMultiplexer::instance().getWriteManager().manageSocket(this);
}

void ConnectedSocket::write(const std::string& junk) {
    writeBuffer += junk;
    SocketMultiplexer::instance().getWriteManager().manageSocket(this);
}


void ConnectedSocket::writeLn(const std::string& junk) {
    writeBuffer += junk + "\r\n";
    SocketMultiplexer::instance().getWriteManager().manageSocket(this);
}


void ConnectedSocket::close() {
    SocketMultiplexer::instance().getReadManager().unmanageSocket(this);
}


void ConnectedSocket::push(const char* junk, int n) {
    if (state == REQUEST || state == HEADER) {
        for (int i = 0; i < n; i++) {
            if (junk[i] != '\r' && junk[i] != '\n') {
                readBuffer += junk[i];
            }
            if (junk[i] == '\n') {
                this->lineRead();
                if (state == BODY) {
                    this->bodyJunk(junk + i + 1, n - i - 1);
                    break;
                }
            }
        }
    } else if (state == BODY) {
        this->bodyJunk(junk, n);
    }
}


void ConnectedSocket::clearReadBuffer() {
    readBuffer.clear();
}


void ConnectedSocket::clearWriteBuffer() {
    writeBuffer.clear();
}


void ConnectedSocket::bodyJunk(const char* junk, int n) {
    if (bodyLength - bodyPointer < n) {
        n = bodyLength - bodyPointer;
    }
    
    memcpy(bodyData + bodyPointer, junk, n);
    bodyPointer += n;
    
    if (bodyPointer == bodyLength)  {
        this->ready();
    }
}


void ConnectedSocket::addHeaderLine(const std::string& line) {
    if (!line.empty()) {
        std::string key;
        std::string token;
        
        std::istringstream tokenStream(line);
        std::getline(tokenStream, key, ':');
        std::getline(tokenStream, token, '\n');
        
        int strBegin = token.find_first_not_of(" \t");
        int strEnd = token.find_last_not_of(" \t");
        int strRange = strEnd - strBegin + 1;
        
        std::transform(key.begin(), key.end(), key.begin(),
                       [](unsigned char c){ return std::tolower(c); });
        
        headerMap[key] = token.substr(strBegin, strRange);
        
        if (key == "content-length") {
            bodyLength = std::stoi(headerMap[key]);
            bodyData = new char[bodyLength];
        }
    }
}

void ConnectedSocket::lineRead() {
    switch(state) {
        case states::REQUEST:
            std::cout << "Request: " << readBuffer << std::endl;
            state = states::HEADER;
            break;
        case states::HEADER: 
            if (readBuffer.empty()) {
                addHeaderLine(line);
                line.clear();
                
                std::map<std::string, std::string>::iterator it;

                if (bodyLength > 0) {
                    state = states::BODY;
                } else {
                    this->ready();
                }
            } else {
                if (isspace(readBuffer.at(0))) {
                    int strBegin = readBuffer.find_first_not_of(" \t");
                    int strEnd = readBuffer.find_last_not_of(" \t");
                    int strRange = strEnd - strBegin + 1;
                    
                    line += " " + readBuffer.substr(strBegin, strRange);
                } else {
                    addHeaderLine(line);
                    line = readBuffer;
                }
            }
            break;
        case states::BODY:
            break;
    };

    this->clearReadBuffer();
}


void ConnectedSocket::send() {
    ssize_t ret = ::send(this->getFd(), writeBuffer.c_str(), (writeBuffer.size() < 4096) ? writeBuffer.size() : 4096, 0);
    
    if (ret >= 0) {
        writeBuffer.erase(0, ret);
        if (writeBuffer.empty()) {
            state = states::REQUEST;
            SocketMultiplexer::instance().getWriteManager().unmanageSocket(this);
        }
    } else {
        state = states::REQUEST;
        SocketMultiplexer::instance().getWriteManager().unmanageSocket(this);
    }
}
