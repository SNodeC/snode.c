#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketReader.h"


class Request;
class Response;
class ConnectedSocket;
class SSLConnectedSocket;

template<typename T>
class BaseServerSocket : public SocketReader {
private:
    BaseServerSocket(const std::function<void (T* cs)>& onConnect,
                 const std::function<void (T* cs)>& onDisconnect,
                 const std::function<void (T* cs, const char*  junk, ssize_t n)>& readProcesor,
                 const std::function<void (int errnum)>& onCsReadError,
                 const std::function<void (int errnum)>& onCsWriteError);

public:
    static BaseServerSocket* instance(const std::function<void (T* cs)>& onConnect,
                                  const std::function<void (T* cs)>& onDisconnect,
                                  const std::function<void (T* cs, const char*  junk, ssize_t n)>& readProcesor,
                                  const std::function<void (int errnum)>& onCsReadError,
                                  const std::function<void (int errnum)>& onCsWriteError);
    ~BaseServerSocket() {}
    
    void listen(in_port_t port, int backlog, const std::function<void (int err)>& onError);
    
    virtual void readEvent();
    
    void disconnect(T* cs);
    
    static void run();
    
    static void stop();
    
private:
    std::function<void (T* cs)> onConnect;
    std::function<void (T* cs)> onDisconnect;
    std::function<void (T* cs, const char* junk, ssize_t n)> readProcessor;

    std::function<void (int errnum)> onCsReadError;
    std::function<void (int errnum)> onCsWriteError;
};

#endif // SERVERSOCKET_H
