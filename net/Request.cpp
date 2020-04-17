#include "Request.h"

#include <sstream>

#include "AcceptedSocket.h"

std::map<std::string, std::string>& Request::header() {
    return acceptedSocket->requestHeader;
}

const char* Request::body() {
    return acceptedSocket->bodyData;
}

int Request::bodySize() {
    return acceptedSocket->bodyLength;
}

bool Request::isGet() {
    return acceptedSocket->requestLine.find("GET") !=  std::string::npos;
}

bool Request::isPost() {
    return acceptedSocket->requestLine.find("POST") != std::string::npos;
}

bool Request::isPut() {
    return acceptedSocket->requestLine.find("PUT") != std::string::npos;
}

const std::string Request::requestURI() {
    std::string method;
    std::string uri;
    
    std::istringstream tokenStream(acceptedSocket->requestLine);
    std::getline(tokenStream, method, ' ');
    std::getline(tokenStream, uri, ' ');
    
    return uri;
}
