#include "Response.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <iostream>

#include "AcceptedSocket.h"

#include "HTTPStatusCodes.h"

Response::Response(AcceptedSocket* as) : acceptedSocket(as) {
}


void Response::status(int status) {
    this->acceptedSocket->responseStatus = status;
}


void Response::set(const std::string& field, const std::string& value) {
    acceptedSocket->responseHeader[field] = value;
}


void Response::append(const std::string& field, const std::string& value) {
    acceptedSocket->responseHeader[field] = acceptedSocket->responseHeader[field] + ", " + value;
}


void Response::send(const std::string& text) {
    acceptedSocket->send(text);
}


void Response::sendFile(const std::string& file) {
    acceptedSocket->sendFile(file);
}


void Response::send(const char* buffer, int n) {
    acceptedSocket->send(buffer, n);
}


void Response::end() {
    acceptedSocket->end();
}
