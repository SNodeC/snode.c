#include "Response.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <iostream>

#include "AcceptedSocket.h"

#include "HTTPStatusCodes.h"

Response::Response(AcceptedSocket* as) : acceptedSocket(as), responseStatus (200), headerSent(false) {
    this->responseHeader["Content-Type"] = "text/html; charset=UTF8";
}


void Response::contentType(const std::string& contentType) {
    this->responseHeader["Content-Type"] = contentType;
}

void Response::contentLength(int length) {
    this->responseHeader["Content-Length"] = std::to_string(length);
}

void Response::status(int status) {
    this->responseStatus = status;
}


void Response::sendHeader() {
    if (!headerSent) {
        acceptedSocket->ConnectedSocket::writeLn("HTTP/1.1 " + std::to_string( responseStatus ) + " " + HTTPStatusCode::reason( responseStatus ));
    
        for (std::map<std::string, std::string>::iterator it = responseHeader.begin(); it != responseHeader.end(); ++it) {
            acceptedSocket->ConnectedSocket::writeLn((*it).first + ": " + (*it).second);
        }
        acceptedSocket->ConnectedSocket::writeLn();
        headerSent = true;
    }
}

void Response::send(const std::string& text) {
    responseHeader["Content-Length"] = std::to_string(text.length());
//    sendHeader();
    acceptedSocket->write(text);
}


void Response::sendFile(const std::string& file) {
//    sendHeader();
    acceptedSocket->sendFile(file);
}


void Response::send(const char* buffer, int n) {
//    sendHeader();
    acceptedSocket->write(buffer, n);
}


void Response::end() {
    acceptedSocket->close();
    headerSent = false;
    responseHeader.clear();
    responseHeader["Content-Type"] = "text/html; charset=UTF8";
    responseStatus = 200;
}
