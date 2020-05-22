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
    
    void sendFile(const std::string& file) const;
    void sendFile(const std::string& file, const std::function<void (int err)>& fn) const;
    
    void end() const;
    
    void set(const std::string& field, const std::string& value) const;
    
    void cookie(const std::string& name, const std::string& value, 
                const std::string& op1 = " ",
                const std::string& op2 = " ",
                const std::string& op3 = " ",
                const std::string& op4 = " ",
                const std::string& op5 = " ",
                const std::string& op6 = " ",
                const std::string& op7 = " "
               ) const;
    
    void redirect(const std::string& name) const;
    
    void redirect(int status, const std::string& name) const;
    
    std::string setOptions(const std::string& op1,
                const std::string& op2,
                const std::string& op3,
                const std::string& op4,
                const std::string& op5,
                const std::string& op6,
                const std::string& op7) const;
    
//    void append(const std::string& field, const std::string& value) const;
    
private:
    HTTPContext* httpContext;
};


#endif // RESPONSE_H
