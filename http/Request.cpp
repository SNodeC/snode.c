#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPContext.h"
#include "Request.h"
#include "httputils.h"


Request::Request(HTTPContext* httpContext)
    : body(httpContext->bodyData)
    , httpContext(httpContext) {
}


const std::string& Request::header(const std::string& key, int i) const {
    std::string tmpKey = key;
    httputils::to_lower(tmpKey);

    if (requestHeader.find(tmpKey) != requestHeader.end()) {
        std::pair<std::multimap<std::string, std::string>::const_iterator, std::multimap<std::string, std::string>::const_iterator> range =
            requestHeader.equal_range(tmpKey);

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
    std::map<std::string, std::string>::const_iterator it;

    if ((it = requestCookies.find(key)) != requestCookies.end()) {
        return it->second;
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
    std::map<std::string, std::string>::const_iterator it;

    if ((it = queryMap.find(key)) != queryMap.end()) {
        return it->second;
    } else {
        return nullstr;
    }
}


const std::string& Request::httpVersion() const {
    return _httpVersion;
}
