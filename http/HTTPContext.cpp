#include "HTTPContext.h"

#include <string.h>

#include <filesystem>
#include <sstream>

#include "ConnectedSocket.h"
#include "HTTPServer.h"
#include "HTTPStatusCodes.h"
#include "MimeTypes.h"

HTTPContext::HTTPContext(HTTPServer* serverSocket, ConnectedSocket* connectedSocket)
: connectedSocket(connectedSocket), serverSocket(serverSocket), bodyData(0), bodyLength(0), state(states::REQUEST), bodyPointer(0), line(""), headerSent(false), responseStatus(200), linestate(READ) {}


void HTTPContext::parseHttpRequest(std::string line) {
    if (state != BODY) {
        readLine(line, [&] (std::string line) -> void {
            switch (state) {
            case REQUEST:
                if (line.empty()) {
                    this->responseStatus = 400;
                    this->responseHeader["Connection"] = "close";
                    connectedSocket->end();
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


void HTTPContext::requestReady() {
    serverSocket->process(Request(this), Response(this));
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
    connectedSocket->send(puffer, size);
}


void HTTPContext::send(const char* puffer, int size) {
    if (responseHeader.find("Content-Type") == responseHeader.end()) {
        responseHeader["Content-Type"] = "application/octet-stream";
    }
    responseHeader["Content-Length"] = size;
    this->sendHeader();
    connectedSocket->send(puffer, size);
    this->end();
}


void HTTPContext::send(const std::string& puffer) {
    if (responseHeader.find("Content-Type") == responseHeader.end()) {
        responseHeader["Content-Type"] = "text/html; charset=utf-8";
    }
    responseHeader["Content-Length"] = puffer.size();
    this->sendHeader();
    connectedSocket->send(puffer);
    this->end();
}


void HTTPContext::sendFile(const std::string& file) {

    std::string absolutFileName = serverSocket->getRootDir() + file;

    if (std::filesystem::exists(absolutFileName)) {
        if (responseHeader.find("Content-Type") == responseHeader.end()) {
            responseHeader["Content-Type"] = MimeTypes::contentType(absolutFileName);
        }
        responseHeader["Content-Length"] = std::to_string(std::filesystem::file_size(absolutFileName));
        this->sendHeader();
        connectedSocket->sendFile(absolutFileName);
        this->end();
    } else {
        this->responseStatus = 404;
        this->responseHeader["Connection"] = "close";
        connectedSocket->end();
        this->end();
    }
}


void HTTPContext::sendHeader() {
    if (!this->headerSent) {
        connectedSocket->send("HTTP/1.1 " + std::to_string( responseStatus ) + " " + HTTPStatusCode::reason( responseStatus )+ "\r\n");

        for (std::map<std::string, std::string>::iterator it = responseHeader.begin(); it != responseHeader.end(); ++it) {
            connectedSocket->send((*it).first + ": " + (*it).second + "\r\n");
        }
        connectedSocket->send("\r\n");
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
            connectedSocket->end();
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

