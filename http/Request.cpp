#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <ctype.h>
#include <algorithm>
#include <sstream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "HTTPContext.h"


Request::Request(HTTPContext* httpContext) : FIELDC(bodyLength), httpContext(httpContext) {}


std::multimap<std::string, std::string>& Request::header() const {
    return this->httpContext->requestHeader;
}


const std::string& Request::header(const std::string& key, int i) const {
    std::string tmpKey = key;
    std::transform(tmpKey.begin(), tmpKey.end(), tmpKey.begin(), ::tolower);
    
    if (this->httpContext->requestHeader.find(tmpKey) != this->httpContext->requestHeader.end()) {
        std::pair<std::multimap<std::string, std::string>::iterator, std::multimap<std::string, std::string>::iterator> range = this->httpContext->requestHeader.equal_range(tmpKey);

        if (std::distance(range.first, range.second) >= i) {
            std::advance(range.first, i);
            return (*(range.first)).second;
        } else {
            return nullstr;
        }
    } else {
        return nullstr;
    }
}

const std::string& Request::cookie(const std::string& key) const {
    if (this->httpContext->requestCookies.find(key) != this->httpContext->requestCookies.end()) {
        return this->httpContext->requestCookies[key];
    } else {
        return nullstr;
    }
}


const char* Request::body() {
    return this->httpContext->bodyData;
}


int Request::bodySize() {
    return this->httpContext->bodyLength;
}


bool Request::isGet() const {
    return this->httpContext->method == "GET";
}


bool Request::isPost() const {
    return this->httpContext->method == "POST";
}


bool Request::isPut() const {
    return this->httpContext->method == "PUT";
}


const std::string& Request::requestURI() const {
    return this->httpContext->path;
}


const std::string& Request::query(std::string key) const {
    if (this->httpContext->queryMap.find(key) != this->httpContext->queryMap.end()) {
        return this->httpContext->queryMap[key];
    } else {
        return nullstr;
    }
}


const std::string& Request::httpVersion() const {
    return this->httpContext->httpVersion;
}


const std::string& Request::fragment() const {
    return this->httpContext->fragment;
}


FIELDI(int, bodyLength);
