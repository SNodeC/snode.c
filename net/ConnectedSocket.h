#ifndef CONNECTEDSOCKET_H
#define CONNECTEDSOCKET_H

#include <string>

#include "InetAddress.h"
#include "SocketWriter.h"
#include "SocketReader.h"


class ServerSocket;

class ConnectedSocket : public SocketReader, public SocketWriter
{
public:
    ConnectedSocket(int csFd, ServerSocket* ss);
    virtual ~ConnectedSocket();
    
    virtual void send(const char* puffer, int size);
    virtual void send(const std::string& junk);
    virtual void sendFile(const std::string& file);
    void end();
    
    InetAddress& getRemoteAddress();
    void setRemoteAddress(const InetAddress& remoteAddress);

protected:
    ServerSocket* serverSocket;
    void clearReadPuffer();
    
    InetAddress remoteAddress;
};

#endif // CONNECTEDSOCKET_H
