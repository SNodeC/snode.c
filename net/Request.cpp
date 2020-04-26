#include <sstream>

#include "Request.h"
#include "AcceptedSocket.h"


std::map<std::string, std::string>& Request::header() {
    return this->acceptedSocket->requestHeader;
}

const char* Request::body() {
    return this->acceptedSocket->bodyData;
}

int Request::bodySize() {
    return this->acceptedSocket->bodyLength;
}

bool Request::isGet() {
    return this->acceptedSocket->requestLine.find("GET") !=  std::string::npos;
}

bool Request::isPost() {
    return this->acceptedSocket->requestLine.find("POST") != std::string::npos;
}

bool Request::isPut() {
    return this->acceptedSocket->requestLine.find("PUT") != std::string::npos;
}

const std::string Request::requestURI() {
    std::string method;
    std::string uri;
    
    std::istringstream tokenStream(this->acceptedSocket->requestLine);
    std::getline(tokenStream, method, ' ');
    std::getline(tokenStream, uri, ' ');
    
    return uri;
}
