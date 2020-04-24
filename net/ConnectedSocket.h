#ifndef CONNECTEDSOCKET_H
#define CONNECTEDSOCKET_H

#include <string>

#include "InetAddress.h"
#include "SocketWriter.h"
#include "SocketReader.h"

class Server;

class ConnectedSocket : public SocketReader, public SocketWriter
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
    Server* serverSocket;
    void clearReadPuffer();
    
    InetAddress remoteAddress;
};

#endif // CONNECTEDSOCKET_H
