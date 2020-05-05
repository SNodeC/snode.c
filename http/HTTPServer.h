#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

class ServerSocket;
class ConnectedSocket;
class Request;
class Response;

class HTTPServer
{
private:
    HTTPServer();

public:
    ~HTTPServer();
    
    static HTTPServer& instance();
    
    void listen(int port);
    void listen(int port, const std::function<void (int err)>& onError);
    
    void destroy();
    
    void process(const Request& request, const Response& response);
    
    void all(const std::string& path, const std::function<void (const Request& req, const Response& res)>& processor);
    
    void get(const std::string& path, const std::function<void (const Request& req, const Response& res)>& processor);
    
    void post(const std::string& path, const std::function<void (const Request& req, const Response& res)>& processor);
    
    void put(const std::string& path, const std::function<void (const Request& req, const Response& res)>& processor);
    
    std::string& getRootDir() {
        return rootDir;
    }
    
    void serverRoot(std::string rootDir) {
        this->rootDir = rootDir;
    }
    
protected:
    std::function<void (const Request& req, const Response& res)> allProcessor;
    std::function<void (const Request& req, const Response& res)> getProcessor;
    std::function<void (const Request& req, const Response& res)> postProcessor;
    std::function<void (const Request& req, const Response& res)> putProcessor;
    
    std::string rootDir;
    
private:
//    ServerSocket* serverSocket;
};

#endif // HTTPSERVER_H
