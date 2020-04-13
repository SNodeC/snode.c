#ifndef SERVERSOCKET_H
#define SERVERSOCKET_H

#include <functional>

#include "Socket.h"


class Request;
class Response;
class ConnectedSocket;

class ServerSocket : public Socket {
private:
    ServerSocket(uint16_t port);
    ServerSocket(const std::string hostname, uint16_t port);

public:
    static ServerSocket* instance(uint16_t port);
    static ServerSocket* instance(const std::string& hostname, uint16_t port);
    
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
        return ::listen(fd, backlog);
    }
    
    ConnectedSocket* accept();
    
private:
    std::function<void (Request& req, Response& res)> allProcessor;
    std::function<void (Request& req, Response& res)> getProcessor;
    std::function<void (Request& req, Response& res)> postProcessor;
    std::function<void (Request& req, Response& res)> putProcessor;
};

#endif // SERVERSOCKET_H
