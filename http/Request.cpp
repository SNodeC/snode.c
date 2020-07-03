#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"

#include "HTTPContext.h"
#include "httputils.h"


Request::Request(HTTPContext* httpContext)
    : httpContext(httpContext) {
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
    return bodyLength;
}


const std::string& Request::query(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it;

    if ((it = queryMap.find(key)) != queryMap.end()) {
        return it->second;
    } else {
        return nullstr;
    }
}


void Request::reset() {
    requestHeader.clear();
    method.clear();
    originalUrl.clear();
    httpVersion.clear();
    path.clear();
    queryMap.clear();

    requestCookies.clear();
    if (body != nullptr) {
        delete[] body;
        body = nullptr;
    }
    bodyLength = 0;
}
