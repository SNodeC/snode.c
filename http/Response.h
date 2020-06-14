#ifndef RESPONSE_H
#define RESPONSE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class HTTPContext;

class Response {
public:
    explicit Response(HTTPContext* httpContext);

    void send(const char* puffer, int n) const;
    void send(const std::string& text) const;

    void sendFile(const std::string& file, const std::function<void(int err)>& fn = nullptr) const;

    void download(const std::string& file, const std::function<void(int err)>& fn = nullptr) const;
    void download(const std::string& file, const std::string& name, const std::function<void(int err)>& fn = nullptr) const;

    void redirect(const std::string& name) const;
    void redirect(int status, const std::string& name) const;

    void sendStatus(int status) const;
    void end() const;

    const Response& status(int status) const;
    const Response& append(const std::string& field, const std::string& value) const;
    const Response& set(const std::string& field, const std::string& value) const;
    const Response& set(const std::map<std::string, std::string>& map) const;
    const Response& cookie(const std::string& name, const std::string& value, const std::map<std::string, std::string>& options = {}) const;
    const Response& clearCookie(const std::string& name, const std::map<std::string, std::string>& options = {}) const;
    const Response& type(const std::string& type) const;

private:
    HTTPContext* httpContext;
};


#endif // RESPONSE_H
