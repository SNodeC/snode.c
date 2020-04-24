#ifndef CONNECTEDSOCKET_H
#define CONNECTEDSOCKET_H

#include <string>

#include "InetAddress.h"
#include "Socket.h"
#include "Writer.h"
#include "Reader.h"

class Server;

class ConnectedSocket : public Socket, public Reader, public Writer
{
public:
    ConnectedSocket(int csFd, Server* ss);
    virtual ~ConnectedSocket();
    
    virtual void send(const char* puffer, int size);
    virtual void send(const std::string& junk);
    virtual void sendFile(const std::string& file);
    
    InetAddress& getRemoteAddress();
    void setRemoteAddress(const InetAddress& remoteAddress);
    
    void end();


protected:
    virtual void writeEvent();
    virtual void readEvent();
    
    Server* serverSocket;
    void clearReadPuffer();
    
    InetAddress remoteAddress;
    
    std::string readPuffer;
    std::string writePuffer;
};

#endif // CONNECTEDSOCKET_H
