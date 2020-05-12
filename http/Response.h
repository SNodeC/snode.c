#ifndef RESPONSE_H
#define RESPONSE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class HTTPContext;

class Response {
public:
    Response(HTTPContext* httpContext);
    ~Response();
    
    void status(int status) const;
    
    void send(const std::string& text) const;
    
    void send(const char* puffer, int n) const;
    
    void sendFile(const std::string& file, const std::function<void (int err)>& fn = 0) const;
    
    void download(const std::string& file, const std::function<void (int err)>& fn = 0) const;
    void download(const std::string& file, const std::string& name, const std::function<void (int err)>& fn = 0) const;
    
    void end() const;
    
    void set(const std::string& field, const std::string& value) const;
    
    void cookie(const std::string& name, const std::string& value) const;
    
    void redirect(const std::string& name) const;
    
    void redirect(int status, const std::string& name) const;
    
    void sendStatus(int status) const;
    
    void type(std::string type) const;
    
//    void append(const std::string& field, const std::string& value) const;
    
private:
    HTTPContext* httpContext;
};


#endif // RESPONSE_H
