#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPContext.h"
#include "Request.h"
#include "httputils.h"


Request::Request(HTTPContext* httpContext)
    : originalUrl(httpContext->originalUrl)
    , body(httpContext->bodyData)
    , path(httpContext->path)
    , httpContext(httpContext) {
}


const std::string& Request::header(const std::string& key, int i) const {
    std::string tmpKey = key;
    httputils::to_lower(tmpKey);

    if (this->httpContext->requestHeader.find(tmpKey) != this->httpContext->requestHeader.end()) {
        std::pair<std::multimap<std::string, std::string>::iterator, std::multimap<std::string, std::string>::iterator> range =
            this->httpContext->requestHeader.equal_range(tmpKey);

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


int Request::bodySize() const {
    return this->httpContext->bodyLength;
}


const std::string& Request::method() const {
    return this->httpContext->method;
}


const std::string& Request::query(const std::string& key) const {
    if (this->httpContext->queryMap.find(key) != this->httpContext->queryMap.end()) {
        return this->httpContext->queryMap[key];
    } else {
        return nullstr;
    }
}


const std::string& Request::httpVersion() const {
    return this->httpContext->httpVersion;
}
