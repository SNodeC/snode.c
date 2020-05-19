#include "Request.h"

#include "AcceptedSocket.h"

std::map<std::string, std::string>& Request::header() {
    return acceptedSocket->headerMap;
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
