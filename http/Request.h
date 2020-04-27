#ifndef REQUEST_H
#define REQUEST_H

#include <map>
#include <string>


class HTTPContext;

class Request {
public:
    Request(HTTPContext* httpContext) : httpContext(httpContext) {
    }
    
    std::map<std::string, std::string>& header();
    
    bool isPost() const;
    bool isGet() const;
    bool isPut() const;
    
    const std::string requestURI() const;
    
    const char* body();
    int bodySize();
    
private:
    HTTPContext* httpContext;
};

#endif // REQUEST_H
