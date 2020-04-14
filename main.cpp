#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <list>
#include <functional>
#include <map>
#include <string.h>

#include "TimerManager.h"
#include "ServerSocket.h"
#include "SocketManager.h"
#include "SocketMultiplexer.h"

#include "Request.h"
#include "Response.h"


int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    
    ServerSocket* serverSocket = ServerSocket::instance(8080);
    
    serverSocket->get([] (Request& req, Response& res) -> void {
        std::map<std::string, std::string>& header = req.header();
        std::map<std::string, std::string>::iterator it;
        
        for (it = header.begin(); it != header.end(); ++it) {
            std::cout << (*it).first << ": " << (*it).second << std::endl;
        }
        
        if (req.bodySize() > 0) {
            const char* body = req.body();
            int bodySize = req.bodySize();
        
            char* str = new char[bodySize + 1];
        
            memcpy(str, body, bodySize);
            str[bodySize] = 0;
        
            std::cout << str << std::endl;
            
            res.send(str);
        } else {
            std::string document;
            
            document = "<!DOCTYPE html>";
            document += "<html><body>";
            document += "<h1>My First Heading</h1>";
            document += "<p>My first paragraph.</p>";
            document += "</body></html>";
            
            res.send(document);
        }
    });
    
    SocketMultiplexer& sm = SocketMultiplexer::instance();

    sm.getReadManager().manageSocket(serverSocket);
    
    while(1) {
        sm.tick();
    }
    
    return 0;
}
