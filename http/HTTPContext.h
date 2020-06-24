#ifndef HTTPCONTEXT_H
#define HTTPCONTEXT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <map>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "Response.h"


class SocketConnection;
class FileReader;
class WebApp;


class ResponseCookie {
public:
    ResponseCookie(const std::string& value, const std::map<std::string, std::string>& options)
        : value(value)
        , options(options) {
    }

private:
    std::string value;
    std::map<std::string, std::string> options;

    friend class HTTPContext;
};


class HTTPContext {
private:
    enum requeststates { REQUEST, HEADER, BODY, ERROR } requestState{REQUEST};

    enum linestate { READ, EOL } lineState{READ};

public:
    HTTPContext(WebApp* webApp, SocketConnection* connectedSocket);

    void receiveRequest(const char* junk, ssize_t junkLen);

    void onReadError(int errnum);
    void onWriteError(int errnum);

private:
    void stopFileReader();

    void send(const char* buffer, int size);
    void send(const std::string& buffer);
    void sendFile(const std::string& file, const std::function<void(int ret)>& onError = nullptr);

    void sendHeader();

    void parseRequest(const char* junk, ssize_t junkLen, const std::function<void(std::string&)>& lineRead,
                      const std::function<void(const char* bodyJunk, int junkLength)>& bodyRead);
    void parseRequestLine(const std::string& line);
    void parseCookie(const std::string& value);
    void addRequestHeader(const std::string& line);

    void requestReady();

    void end();
    void prepareForRequest();

    char* bodyData{nullptr};
    int bodyLength{0};

    int responseStatus{0};

    /* Request-Line */
    std::string method;
    std::string originalUrl;
    std::string httpVersion;

    std::string path;

    std::map<std::string, std::string> queryMap;
    std::multimap<std::string, std::string> requestHeader;
    std::map<std::string, std::string> responseHeader;
    std::map<std::string, std::string> requestCookies;
    std::map<std::string, ResponseCookie> responseCookies;

    std::map<std::string, std::string> params;

    void enqueue(const char* buf, size_t len);
    void enqueue(const std::string& str);

    SocketConnection* connectedSocket;
    FileReader* fileReader{nullptr};
    WebApp* webApp;

    std::string headerLine;
    int bodyPointer{0};

    bool headerSend{false};

    Request request;
    Response response;

    int contentLength{0};
    int sendLen{0};

    bool requestInProgress{false};

    friend class Response;
    friend class Request;
};

#endif // HTTPCONTEXT_H
