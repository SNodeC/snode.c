#ifndef SOCKETCONNECTIONINTERFACE_H
#define SOCKETCONNECTIONINTERFACE_H

#include <functional>

#include "InetAddress.h"

class SocketConnectionInterface
{public:
    virtual ~SocketConnectionInterface() = default;
    
    virtual void setContext(void* context) = 0;
    virtual void* getContext() = 0;
    
    virtual void send(const char* puffer, int size) = 0;
    virtual void send(const std::string& junk) = 0;
    virtual void sendFile(const std::string& file, const std::function<void (int ret)>& onError) = 0;
    virtual void end() = 0;
    
    virtual InetAddress& getRemoteAddress() = 0;
    virtual void setRemoteAddress(const InetAddress& remoteAddress) = 0;
    
protected:/*
    SocketServerInterface* serverSocket;
    
    void* context;
    
    InetAddress remoteAddress;
    
    FileReader* fileReader;*/
};

#endif // SOCKETCONNECTIONINTERFACE_H
