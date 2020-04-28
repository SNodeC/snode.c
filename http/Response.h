#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>


class HTTPContext;

class Response {
public:
    Response(HTTPContext* httpContext);
    ~Response();
    
    void status(int status);
    
    void send(const std::string& text) const;
    
    void send(const char* puffer, int n) const;
    
    void sendFile(const std::string& file) const;
    
    void end() const;
    
    void set(const std::string& field, const std::string& value) const;
    
    void append(const std::string& field, const std::string& value) const;
    
private:
    HTTPContext* httpContext;
};


#endif // RESPONSE_H
