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
