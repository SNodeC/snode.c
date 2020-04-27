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
    
    bool isPost();
    bool isGet();
    bool isPut();
    
    const std::string requestURI();
    
    const char* body();
    int bodySize();
    
private:
    HTTPContext* httpContext;
};

#endif // REQUEST_H
