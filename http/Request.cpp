#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"

#include "httputils.h"

const std::string& Request::header(const std::string& key, int i) const {
    std::string tmpKey = key;
    httputils::to_lower(tmpKey);

    if (headers->find(tmpKey) != headers->end()) {
        std::pair<std::multimap<std::string, std::string>::const_iterator, std::multimap<std::string, std::string>::const_iterator> range =
            headers->equal_range(tmpKey);

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

    if ((it = cookies->find(key)) != cookies->end()) {
        return it->second;
    } else {
        return nullstr;
    }
}

int Request::bodyLength() const {
    return contentLength;
}

const std::string& Request::query(const std::string& key) const {
    std::map<std::string, std::string>::const_iterator it;

    if ((it = queries->find(key)) != queries->end()) {
        return it->second;
    } else {
        return nullstr;
    }
}

void Request::reset() {
    method.clear();
    originalUrl.clear();
    httpVersion.clear();
    path.clear();
    body = nullptr;
    contentLength = 0;
    MultibleAttributeInjector::reset();
}
