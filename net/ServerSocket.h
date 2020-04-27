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
                 std::function<void (ConnectedSocket* cs, std::string line)> rp);
    
    ServerSocket(uint16_t port, 
                 std::function<void (ConnectedSocket* cs)> onConnect,
                 std::function<void (ConnectedSocket* cs)> onDisconnect,
                 std::function<void (ConnectedSocket* cs, std::string line)> readProcesor);

public:
    static ServerSocket& instance(uint16_t port, 
                                  std::function<void (ConnectedSocket* cs)> onConnect,
                                  std::function<void (ConnectedSocket* cs)> onDisconnect,
                                  std::function<void (ConnectedSocket* cs, std::string line)> readProcesor);
    
    void process(Request& request, Response& response);
    
    void all(std::function<void (Request& req, Response& res)> processor) {
        allProcessor = processor;
    }
    
    void get(std::function<void (Request& req, Response& res)> processor) {
        getProcessor = processor;
    }
    
    void post(std::function<void (Request& req, Response& res)> processor) {
        postProcessor = processor;
    }
    
    void put(std::function<void (Request& req, Response& res)> processor) {
        putProcessor = processor;
    }
    
    
    int listen(int backlog) {
        return ::listen(this->getFd(), backlog);
    }
    
    void serverRoot(std::string rootDir);
    
    std::string& getRootDir();
    
    virtual void readEvent();
    virtual void readEvent1();
    
    void disconnect(ConnectedSocket* cs);
    
    static void run();
    
private:
    std::function<void (Request& req, Response& res)> allProcessor;
    std::function<void (Request& req, Response& res)> getProcessor;
    std::function<void (Request& req, Response& res)> postProcessor;
    std::function<void (Request& req, Response& res)> putProcessor;
    
    std::function<void (ConnectedSocket* cs)> onConnect;
    std::function<void (ConnectedSocket* cs)> onDisconnect;
    std::function<void (ConnectedSocket* cs, std::string line)> readProcessor;
    
    std::string rootDir;
};

#endif // SERVERSOCKET_H
