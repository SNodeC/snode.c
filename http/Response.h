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
    Cookie() {}
    Cookie(const std::string& name, const std::string& value) : name(name), value(value) {}
    void Domain(const std::string& domain);
    void Path(const std::string& path);
    void MaxAge(const std::string& maxAge);
    void Expires(const std::string& expires);
    void MakeEssential();
    void SetSameSite(const bool strict);
    void MakeSecure();
    void MakeHttpOnly();
    void reset();

    const std::string &getName() const;
    const std::string &getValue() const;
    const std::map<std::string, std::string> &getOptions() const;

protected:
    std::string name;
    std::string value;
    std::map<std::string, std::string> options;


protected:

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
