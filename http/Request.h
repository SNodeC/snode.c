#ifndef REQUEST_H
#define REQUEST_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "AttributeInjector.h"

class HTTPServerContext;

class Request : public utils::MultibleAttributeInjector {
private:
    explicit Request();

public:
    const std::string& header(const std::string& key, int i = 0) const;
    const std::string& cookie(const std::string& key) const;
    const std::string& query(const std::string& key) const;
    int bodyLength() const;

    // Properties
    std::string originalUrl;
    std::string httpVersion;
    std::string url;
    char* body = nullptr;
    std::string path;
    std::string method;
    int contentLength = 0;

protected:
    void reset();

    std::map<std::string, std::string> queryMap;
    const std::map<std::string, std::string>* requestHeader = nullptr;
    const std::map<std::string, std::string>* requestCookies = nullptr;

private:
    std::string nullstr = "";

    friend class HTTPServerContext;
    friend class Response;
};

#endif // REQUEST_H
