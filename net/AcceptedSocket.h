#ifndef ACCEPTEDSOCKET_H
#define ACCEPTEDSOCKET_H

#include "ConnectedSocket.h"

#include <map>

#include "Request.h"
#include "Response.h"

class ServerSocket;

class AcceptedSocket : public ConnectedSocket
{
public:
    AcceptedSocket(int csFd, ServerSocket* ss);
    ~AcceptedSocket();
    
    virtual void ready();
    
    virtual void read(const char* junk, int n);
    
    virtual void reset();
    
protected:
    ServerSocket* serverSocket;
    
    void bodyJunk(const char* junk, int n);
    void lineRead();
    void addHeaderLine(const std::string& line);
    
    std::map<std::string, std::string> headerMap;
    
private:
    Request request;
    Response response;
    
    char* bodyData;
    int bodyLength;
    
    enum states {
        REQUEST,
        HEADER,
        BODY
    } state;
    
    int bodyPointer;
    std::string line;
    
friend class Response;
friend class Request;
};

#endif // ACCEPTEDSOCKET_H
