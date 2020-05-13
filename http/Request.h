#ifndef REQUEST_H
#define REQUEST_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

class HTTPContext;

#define FIELD(T, prop) \
class C ## prop {\
public:\
    C ## prop(HTTPContext* httpContext) : httpContext(httpContext) {}\
\
    operator T() const;\
    T operator=(T i) const;\
\
private:\
    HTTPContext* httpContext;\
};\
\
public:\
    C ## prop prop;\
\
private:

#define FIELDI(T, prop) \
Request::C ## prop::operator T() const {\
    return this->httpContext->prop;\
}\
\
T Request::C ## prop::operator=(T i) const {\
    this->httpContext->prop = i;\
\
    return i;\
}

#define FIELDC(prop) prop(httpContext)


class Request {
private:
    FIELD(int, bodyLength);
    
public:
    Request(HTTPContext* httpContext);
    
    std::multimap<std::string, std::string>& header() const;
    const std::string& header(const std::string& key, int i = 0) const;
    const std::string& cookie(const std::string& key) const;
    
    bool isPost() const;
    bool isGet() const;
    bool isPut() const;
    
    const std::string& requestURI() const;
    const std::string& query(std::string key) const;
    const std::string& httpVersion() const;
    const std::string& fragment() const;
    
    const char* body();
    int bodySize();
    
private:
    HTTPContext* httpContext;
    
    std::string nullstr = "";
};

#endif // REQUEST_H
