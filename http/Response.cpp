#include "Response.h"
#include "HTTPContext.h"
#include "HTTPStatusCodes.h"


Response::Response(HTTPContext* httpContext) : httpContext(httpContext) {
}


Response::~Response() {
    this->httpContext->reset();
}


void Response::status(int status) {
    this->httpContext->responseStatus = status;
}


void Response::set(const std::string& field, const std::string& value) const {
    this->httpContext->responseHeader.insert({field, value});
}

/*
void Response::append(const std::string& field, const std::string& value) const {
    this->httpContext->responseHeader[field] = httpContext->responseHeader[field] + ", " + value;
}
*/


void Response::cookie(const std::string& name, const std::string& value, const std::map<std::string, std::string>& options) const {
    // class Cookie cookie(value, const std::map<std::string opt, std::string optvalue>& options)
    Cookie cookie(value, options);
    this->httpContext->responseCookies.insert({name, cookie});
}

void Response::send(const std::string& text) const {
    this->httpContext->send(text);
}


void Response::sendFile(const std::string& file) const {
    this->httpContext->sendFile(file, 0);
}


void Response::sendFile(const std::string& file, const std::function<void (int err)>& fn) const {
    this->httpContext->sendFile(file, fn);
}


void Response::send(const char* puffer, int n) const {
    this->httpContext->send(puffer, n);
}


void Response::end() const {
    this->httpContext->end();
}

void Cookie::Domain(const std::string &domain) {
    this->options.insert({ "Domain", domain});
}

void Cookie::Path(const std::string &path) {
    this->options.insert({ "Path", path});
}

void Cookie::MaxAge(const std::string &maxAge) {
    this->options.insert({ "Max-Age", maxAge});
}

void Cookie::Expires(const std::string &expires) {
    this->options.insert({ "Expires", expires});
}

void Cookie::MakeEssential() {
    this->options.insert({ "IsEssential", "true"});
}

void Cookie::MakeSecure() {
    this->options.insert({ "Secure", "true"});
}

void Cookie::MakeHttpOnly() {
    this->options.insert({ "HttpOnly", "true"});
}

void Cookie::SetSameSite(const bool strict) {
    this->options.insert( { "SameSite", strict ? "Strict" : "None" });
}

void Cookie::reset() {
    this->value = "";
    this->options.clear();
}

const std::string &Cookie::getName() const {
    return name;
}

const std::string &Cookie::getValue() const {
    return value;
}

const std::map<std::string, std::string> &Cookie::getOptions() const {
    return options;
}
