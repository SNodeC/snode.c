#ifndef CONNECTEDSOCKET_H
#define CONNECTEDSOCKET_H

#include <string>

#include "InetAddress.h"
#include "Socket.h"


class ConnectedSocket : public Socket
{
public:
    ConnectedSocket(int csFd);
    virtual ~ConnectedSocket();
    
    virtual void write(const char* buffer, int size);
    virtual void write(const std::string& junk);
    virtual void writeLn(const std::string& junk = "");
    
    InetAddress& getLocalAddress();
    InetAddress& getRemoteAddress();
    void setRemoteAddress(const InetAddress& remoteAddress);
    
    virtual void read(const char* junk, int n) = 0;
    
    void close();
    
    void send();
    
    virtual void ready() = 0;
    
    virtual void reset() = 0;
    
protected:
    void clearReadBuffer();
    void clearWriteBuffer();
    
    InetAddress remoteAddress;
     
    std::string readBuffer;
    std::string writeBuffer;
};

#endif // CONNECTEDSOCKET_H
