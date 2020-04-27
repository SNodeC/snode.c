#ifndef HTTPCONTEXT_H
#define HTTPCONTEXT_H

#include <functional>
#include <string>
#include <map>

#include "Request.h"
#include "Response.h"

class ConnectedSocket;
class HTTPServer;

class HTTPContext
{
public:
    HTTPContext(HTTPServer* serverSocket, ConnectedSocket* connectedSocket);
    
    void parseHttpRequest(std::string line);
    void readLine(std::string readPuffer, std::function<void (std::string)> lineRead);
    void requestReady();
    
    void addRequestHeader(std::string& line);  
    void sendHeader();
    
    void send(const char* puffer, int size);
    void send(const std::string& data);
    void sendJunk(const char* puffer, int size);
    void sendFile(const std::string& file);
    
    void end();

protected:    
    ConnectedSocket* connectedSocket;
    HTTPServer* serverSocket;
    
    std::string requestLine;
    std::map<std::string, std::string> requestHeader;
    std::map<std::string, std::string> responseHeader;
    
    
    char* bodyData;
    int bodyLength;
    
    enum states {
        REQUEST,
        HEADER,
        BODY,
        ERROR
    } state;
    
    
    int bodyPointer;
    std::string line;
    
    bool headerSent;
    int responseStatus;
    
    enum linestate {
        READ,
        EOL
    } linestate;
    
    std::string headerLine;
    
    friend class Response;
    friend class Request;
};

#endif // HTTPCONTEXT_H
