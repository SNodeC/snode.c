#ifndef BASECONNECTEDSOCKET_H
#define BASECONNECTEDSOCKET_H

#include "SocketConnection.h"

class FileReader;
class SocketServer;

template<typename R, typename W>
class SocketConnectionBase : public SocketConnection, public R, public W
{
public:
    
    virtual ~SocketConnectionBase();

    void setContext(void* context) {
        this->context = context;
    }

    void* getContext() {
        return this->context;
    }

    virtual void send(const char* puffer, int size);
    virtual void send(const std::string& junk);
    virtual void sendFile(const std::string& file, const std::function<void (int ret)>& onError);
    virtual void end();

    virtual InetAddress& getRemoteAddress();
    virtual void setRemoteAddress(const InetAddress& remoteAddress);

protected:
    SocketConnectionBase(int csFd,
                         SocketServer* serverSocket,
                         const std::function<void (SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                         const std::function<void (int errnum)>& onReadError,
                         const std::function<void (int errnum)>& onWriteError
    );
    
    SocketServer* serverSocket;
    
    void* context;
    
    InetAddress remoteAddress;

    FileReader* fileReader;
};

#endif // BASECONNECTEDSOCKET_H
