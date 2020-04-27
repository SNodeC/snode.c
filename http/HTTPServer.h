#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <functional>
#include <iostream>
#include <list>

#include "ServerSocket.h"
#include "Request.h"
#include "Response.h"

class ConnectedSocket;

class HTTPServer
{
public:
    HTTPServer(int port);
    
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
    
    void process(Request& request, Response& response);
    
    std::string& getRootDir() {
        return rootDir;
    }
    
    void serverRoot(std::string rootDir) {
        this->rootDir = rootDir;
    }
    
protected:
    std::function<void (Request& req, Response& res)> allProcessor;
    std::function<void (Request& req, Response& res)> getProcessor;
    std::function<void (Request& req, Response& res)> postProcessor;
    std::function<void (Request& req, Response& res)> putProcessor;
    
    std::string rootDir;
};

#endif // HTTPSERVER_H
