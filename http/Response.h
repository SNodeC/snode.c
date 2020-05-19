#ifndef RESPONSE_H
#define RESPONSE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class HTTPContext;

class Cookie {
public:
    Cookie(const std::string& value, const std::map<std::string, std::string>& options) : value(value), options(options) {}

protected:
    const std::string value;
    const std::map<std::string, std::string> options;

friend class HTTPContext;
};

class Response {
public:
    Response(HTTPContext* httpContext);
    ~Response();
    
    void status(int status);
    
    void send(const std::string& text) const;
    
    void send(const char* puffer, int n) const;
    
    void sendFile(const std::string& file) const;
    void sendFile(const std::string& file, const std::function<void (int err)>& fn) const;
    
    void end() const;
    
    void set(const std::string& field, const std::string& value) const;
    
    void cookie(const std::string& name, const std::string& value, const std::map<std::string, std::string>& options = {}) const;
    
//    void append(const std::string& field, const std::string& value) const;
    
private:
    HTTPContext* httpContext;
};


#endif // RESPONSE_H
