#ifndef HTTPCONTEXT_H
#define HTTPCONTEXT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "Response.h"

class ConnectedSocket;
class HTTPServer;

class HTTPContext
{
public:
    HTTPContext(HTTPServer* httpServer, ConnectedSocket* connectedSocket);
    
    void send(const char* puffer, int size);
    void send(const std::string& data);
//    void sendJunk(const char* puffer, int size);
    void sendFile(const std::string& file, const std::function<void (int ret)>& fn);
    
    void end();

protected:
    void reset();
    
    void receiveRequest(const char* junk, ssize_t n);
    void parseRequest(const char* junk, ssize_t, const std::function<void (std::string&)>& lineRead);
    
    void parseRequestLine(const std::string& line);
    void parseCookie(const std::string& value);
    
    void requestReady();
    
    void addRequestHeader(const std::string& line);  
    void sendHeader();
    
    ConnectedSocket* connectedSocket;
    HTTPServer* httpServer;
    
    char* bodyData;
    int bodyLength;
    
    enum states {
        REQUEST,
        HEADER,
        BODY,
        ERROR
    } requestState;
    
    
    int bodyPointer;
    std::string line;
    
    int responseStatus;
    
    enum linestate {
        READ,
        EOL
    } linestate;
    
    std::string headerLine;
    
    /* Request-Line */
    std::string method;
    std::string requestUri;
    std::string httpVersion;
    
    std::string path;
    std::string fragment;
    
    std::map<std::string, std::string> queryMap;
    std::multimap<std::string, std::string> requestHeader;
    std::multimap<std::string, std::string> responseHeader;
    std::map<std::string, std::string> requestCookies;
    std::map<std::string, std::string> responseCookies;
    
    friend class Response;
    friend class Request;
    friend class HTTPServer;
    
    Request request;
    Response response;
};

#endif // HTTPCONTEXT_H
