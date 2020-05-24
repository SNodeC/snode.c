#ifndef HTTPCONTEXT_H
#define HTTPCONTEXT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "Response.h"

class SocketConnectionBase;
class WebApp;


class ResponseCookie {
public:
    ResponseCookie(const std::string& value, const std::map<std::string, std::string>& options) : value(value), options(options) {}

protected:
    std::string value;
    std::map<std::string, std::string> options;

    friend class HTTPContext;
};


class HTTPContext
{
protected:
    enum requeststates {
        REQUEST,
        HEADER,
        BODY,
        ERROR
    } requestState;

    enum linestate {
        READ,
        EOL
    } lineState;

public:
    HTTPContext(WebApp* httpServer, SocketConnectionBase* connectedSocket);
    void receiveRequest(const char* junk, ssize_t n);

protected:
    void send(const char* puffer, int size);
    void send(const std::string& data);
    void sendFile(const std::string& file, const std::function<void (int ret)>& fn);

    void sendHeader();

    void parseRequest(const char* junk, ssize_t, const std::function<void (std::string&)>& lineRead, const std::function<void (const char* bodyJunk, int junkLength)> bodyRead);
    void parseRequestLine(const std::string& line);
    void parseCookie(const std::string& value);
    void addRequestHeader(const std::string& line);

    void requestReady();

    void end();
    void reset();

    char* bodyData;
    int bodyLength;


    int responseStatus;

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

    friend class Response;
    friend class Request;

private:
    SocketConnectionBase* connectedSocket;
    WebApp* httpServer;

    std::string headerLine;
    int bodyPointer;

    bool headerSend;

    Request request;
    Response response;
};

#endif // HTTPCONTEXT_H
