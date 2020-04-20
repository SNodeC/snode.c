#ifndef ACCEPTEDSOCKET_H
#define ACCEPTEDSOCKET_H

#include "ConnectedSocket.h"

#include <map>

#include "Request.h"
#include "Response.h"

class Server;

class AcceptedSocket : public ConnectedSocket
{
public:
    AcceptedSocket(int csFd, Server* ss);
    ~AcceptedSocket();
    
    
    virtual void send(const char* buffer, int size);
    virtual void send(const std::string& junk);
    virtual void sendFile(const std::string& file);
    
    virtual void end(bool force = false);
    
    
protected:
    virtual void readEvent();
    virtual void writeEvent();
    
    void junkRead(const char* junk, int n);
    void lineRead();
    void addRequestHeader(const std::string& line);
    void bodyJunk(const char* junk, int n);
    void requestReady();
    
    void sendHeader();

    
private:
    Request request;
    Response response;
    
    std::string requestLine;
    std::map<std::string, std::string> requestHeader;
    std::map<std::string, std::string> responseHeader;
    
    
    char* bodyData;
    int bodyLength;
    
    enum states {
        REQUEST,
        HEADER,
        BODY
    } state;
    
    int bodyPointer;
    std::string line;
    
    bool headerSent;
    int responseStatus;
    
    
friend class Response;
friend class Request;
};

#endif // ACCEPTEDSOCKET_H

