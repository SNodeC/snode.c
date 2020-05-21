#ifndef REQUEST_H
#define REQUEST_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

class HTTPContext;

class Request {
private:
    
public:
    Request(HTTPContext* httpContext);
    
    std::multimap<std::string, std::string>& header() const;
    const std::string& header(const std::string& key, int i = 0) const;
    const std::string& cookie(const std::string& key) const;

    const std::string& query(std::string key) const;
    const std::string& httpVersion() const;
    const std::string& fragment() const;
    
    const std::string& method() const;
    
    int bodySize() const;
    
// Properties
    const std::string& originalUrl;
    mutable std::string url;
    char*& body;
    const std::string& path;
    
private:
    HTTPContext* httpContext;
    
    std::string nullstr = "";
};

#endif // REQUEST_H
