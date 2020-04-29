#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include <functional>

#include "SocketReader.h"


class Request;
class Response;
class ConnectedSocket;

class ServerSocket : public SocketReader {
private:
    ServerSocket(std::function<void (ConnectedSocket* cs)> onConnect,
                 std::function<void (ConnectedSocket* cs)> onDisconnect,
                 std::function<void (ConnectedSocket* cs, std::string line)> readProcesor);
    
    ServerSocket(uint16_t port, 
                 std::function<void (ConnectedSocket* cs)> onConnect,
                 std::function<void (ConnectedSocket* cs)> onDisconnect,
                 std::function<void (ConnectedSocket* cs, std::string line)> readProcesor);

public:
    static ServerSocket* instance(uint16_t port, 
                                  std::function<void (ConnectedSocket* cs)> onConnect,
                                  std::function<void (ConnectedSocket* cs)> onDisconnect,
                                  std::function<void (ConnectedSocket* cs, std::string line)> readProcesor);
    
    int listen(int backlog) {
        return ::listen(this->getFd(), backlog);
    }
    
    virtual void readEvent();
    
    void disconnect(ConnectedSocket* cs);
    
    static void run();
    
private:
    std::function<void (ConnectedSocket* cs)> onConnect;
    std::function<void (ConnectedSocket* cs)> onDisconnect;
    std::function<void (ConnectedSocket* cs, std::string line)> readProcessor;
};

#endif // SERVERSOCKET_H
