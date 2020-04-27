#include "Response.h"
#include "HTTPContext.h"
#include "HTTPStatusCodes.h"


Response::Response(HTTPContext* httpContext) : httpContext(httpContext) {
}


void Response::status(int status) {
    this->httpContext->responseStatus = status;
}


void Response::set(const std::string& field, const std::string& value) {
    this->httpContext->responseHeader[field] = value;
}


void Response::append(const std::string& field, const std::string& value) {
    this->httpContext->responseHeader[field] = httpContext->responseHeader[field] + ", " + value;
}


void Response::send(const std::string& text) const {
    this->httpContext->send(text);
}


void Response::sendFile(const std::string& file) const {
    this->httpContext->sendFile(file);
}


void Response::send(const char* puffer, int n) const {
    this->httpContext->send(puffer, n);
}


void Response::end() {
    this->httpContext->end();
}
