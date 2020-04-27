#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <functional>

#include "Request.h"
#include "Response.h"

class ServerSocket;
class ConnectedSocket;

class HTTPServer
{
private:
    HTTPServer(int port);

public:    
    static HTTPServer& instance(int port);
    
    void destroy();
    
    void process(const Request& request, const Response& response);
    
    void all(std::function<void (const Request& req, const Response& res)> processor) {
        allProcessor = processor;
    }
    
    void get(std::function<void (const Request& req, const Response& res)> processor) {
        getProcessor = processor;
    }
    
    void post(std::function<void (const Request& req, const Response& res)> processor) {
        postProcessor = processor;
    }
    
    void put(std::function<void (const Request& req, const Response& res)> processor) {
        putProcessor = processor;
    }
    
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
};

#endif // HTTPSERVER_H
