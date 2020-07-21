#ifndef REQUEST_H
#define REQUEST_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "AttributeInjector.h"

class HTTPContext;


class Request : public utils::MultibleAttributeInjector {
private:
    explicit Request();

public:
    const std::string& header(const std::string& key, int i = 0) const;
    const std::string& cookie(const std::string& key) const;
    const std::string& query(const std::string& key) const;
    const std::string& fragment() const;
    int bodyLength() const;

    // Properties
    std::string originalUrl;
    std::string httpVersion;
    mutable std::string url;
    char* body{nullptr};
    std::string path;
    std::string method;
    int contentLength = 0;

protected:
    std::map<std::string, std::string> queryMap;
    std::map<std::string, std::string> requestHeader;
    std::map<std::string, std::string> requestCookies;

private:
    void reset();

    std::string nullstr = "";

    friend class HTTPContext;
    friend class Response;
};

#endif // REQUEST_H
