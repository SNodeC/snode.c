#ifndef CONNECTEDSOCKET_H
#define CONNECTEDSOCKET_H

#include "InetAddress.h"
#include "Socket.h"
#include "Request.h"
#include "Response.h"

#include <map>

class ConnectedSocket : public Socket
{
public:
    ConnectedSocket(int csFd);
    virtual ~ConnectedSocket();
    
    void write(const char* buffer, int size);
    void write(const std::string& junk);
    void writeLn(const std::string& junk = "");
    
    InetAddress& getLocalAddress();
    InetAddress& getRemoteAddress();
    void setRemoteAddress(const InetAddress& remoteAddress);
    
    virtual void push(const char* junk, int n);
    
    virtual void close();
    
    void send();
    
    virtual void ready() = 0;
    
    
protected:
    void lineRead();
    void addHeaderLine(const std::string& line);
    void bodyJunk(const char* junk, int n);
    void clearReadBuffer();
    void clearWriteBuffer();
    
    InetAddress remoteAddress;
    std::string readBuffer;
    std::string writeBuffer;
    
    std::map<std::string, std::string> headerMap;
    
    enum states {
        REQUEST,
        HEADER,
        BODY
    } state;
    
    char* bodyData;
    int bodyLength;
    
private:
    int bodyPointer;
    std::string line;
    
friend class Response;
friend class Request;
};

#endif // CONNECTEDSOCKET_H
