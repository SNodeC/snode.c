#include "AcceptedSocket.h"

#include <algorithm>
#include <iostream>
#include <sstream>

#include <string.h>

#include "ServerSocket.h"


AcceptedSocket::AcceptedSocket(int csFd, ServerSocket* ss) : ConnectedSocket(csFd), serverSocket(ss), request(this), response(this), bodyData(0), bodyLength(0), state(states::REQUEST), bodyPointer(0), line("")  {
}


AcceptedSocket::~AcceptedSocket() {    
    if (bodyData != 0) {
        delete bodyData;
    }
}


void AcceptedSocket::ready() {
    serverSocket->process(request, response);
}


void AcceptedSocket::read(const char* junk, int n) {
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


void AcceptedSocket::bodyJunk(const char* junk, int n) {
    if (bodyLength - bodyPointer < n) {
        n = bodyLength - bodyPointer;
    }
    
    memcpy(bodyData + bodyPointer, junk, n);
    bodyPointer += n;
    
    if (bodyPointer == bodyLength)  {
        this->ready();
    }
}


void AcceptedSocket::addHeaderLine(const std::string& line) {
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


void AcceptedSocket::lineRead() {
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


void AcceptedSocket::reset() {
    state = states::REQUEST;
}
