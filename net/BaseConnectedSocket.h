#ifndef BASECONNECTEDSOCKET_H
#define BASECONNECTEDSOCKET_H

#include "InetAddress.h"
#include "FileReader.h"

class BaseConnectedSocket
{
public:
    virtual ~BaseConnectedSocket() = default;
    
    void setContext(void* context) {
        this->context = context;
    }
    
    void* getContext() {
        return this->context;
    }
    
    virtual void send(const char* puffer, int size) = 0;
    virtual void send(const std::string& junk) = 0;
    virtual void sendFile(const std::string& file, const std::function<void (int ret)>& onError) = 0;
    virtual void end() = 0;
    
    virtual InetAddress& getRemoteAddress() = 0;
    virtual void setRemoteAddress(const InetAddress& remoteAddress) = 0;
    
protected:
    void* context;
    
    InetAddress remoteAddress;

//    FileReader* fileReader;
};

#endif // BASECONNECTEDSOCKET_H
