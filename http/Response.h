#ifndef RESPONSE_H
#define RESPONSE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class FileReader;
class HTTPServerContext;

class Response {
private:
    explicit Response(HTTPServerContext* httpContext);

public:
    void send(const char* buffer, size_t size);
    void send(const std::string& text);

    void sendFile(const std::string& file, const std::function<void(int err)>& onError = nullptr);
    void download(const std::string& file, const std::function<void(int err)>& onError = nullptr);
    void download(const std::string& file, const std::string& name, const std::function<void(int err)>& onError = nullptr);

    void redirect(const std::string& name);
    void redirect(int status, const std::string& name);

    void sendStatus(int status);
    void end();

    Response& status(int status);
    Response& append(const std::string& field, const std::string& value);
    Response& set(const std::string& field, const std::string& value, bool overwrite = false);
    Response& set(const std::map<std::string, std::string>& map, bool overwrite = false);
    Response& cookie(const std::string& name, const std::string& value, const std::map<std::string, std::string>& options = {});
    Response& clearCookie(const std::string& name, const std::map<std::string, std::string>& options = {});
    Response& type(const std::string& type);

    bool headersSent = false;

protected:
    mutable size_t contentLength;

    void enqueue(const char* buf, size_t len);
    void enqueue(const std::string& str);

    void sendHeader();
    void stop();
    void reset();

private:
    class ResponseCookie {
    public:
        ResponseCookie(const std::string& value, const std::map<std::string, std::string>& options)
            : value(value)
            , options(options) {
        }

    protected:
        std::string value;
        std::map<std::string, std::string> options;

        friend class Response;
    };

    HTTPServerContext* httpContext;
    FileReader* fileReader = nullptr;

    size_t contentSent = 0;
    bool headersSentInProgress = false;

    int responseStatus = 0;
    std::map<std::string, std::string> responseHeader;
    std::map<std::string, ResponseCookie> responseCookies;

    friend class HTTPServerContext;
};


#endif // RESPONSE_H
