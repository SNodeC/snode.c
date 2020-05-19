#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Router.h"

class ServerSocket;
class ConnectedSocket;
class Request;
class Response;

class HTTPServer : public Router
{
private:
    HTTPServer(const std::string& serverRoot);

public:
    ~HTTPServer();
    
    static HTTPServer& instance(const std::string& serverRoot = "./");
    
    void listen(int port);
    void listen(int port, const std::function<void (int err)>& onError);
    
    void destroy();
    
    std::string& getRootDir() {
        return rootDir;
    }
    
    void serverRoot(std::string rootDir) {
        this->rootDir = rootDir;
        if (this->rootDir.back() != '/') {
            this->rootDir += '/';
        }
    }

    std::string rootDir;
};

#endif // HTTPSERVER_H
