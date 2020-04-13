#include "Request.h"

#include "ConnectedSocket.h"

std::map<std::string, std::string>& Request::header() {
    return connectedSocket->headerMap;
}

const char* Request::body() {
    return connectedSocket->bodyData;
}

int Request::bodySize() {
    return connectedSocket->bodyLength;
}
