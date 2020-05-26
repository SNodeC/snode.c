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

#include "ContinousTimer.h"
#include <FileReader.h>


int main(int argc, char **argv) {
    signal(SIGPIPE, SIG_IGN);
    
    ServerSocket* serverSocket = ServerSocket::instance(8080);
    
    serverSocket->get([] (Request& request, Response& response) -> void {
        std::map<std::string, std::string>& header = request.header();
        std::map<std::string, std::string>::iterator it;
        
        for (it = header.begin(); it != header.end(); ++it) {
            std::cout << (*it).first << ": " << (*it).second << std::endl;
        }
        
        if (request.bodySize() > 0) {
            const char* body = request.body();
            int bodySize = request.bodySize();
        
            char* str = new char[bodySize + 1];
        
            memcpy(str, body, bodySize);
            str[bodySize] = 0;
        
            std::cout << str << std::endl;
            
            response.send(str);
        } else {
            std::string document;
            
            document = "<!DOCTYPE html>";
            document += "<html><body>";
            document += "<h1>My First Heading</h1>";
            document += "<p>My first paragraph.</p>";
            document += "</body></html>";
            
            response.send(document);
        }
        
        const char* fileToRead = "./index.html";
        response.header();
        FileReader::read(fileToRead, 
            [&] (unsigned char* data, int length) -> void
            {
                if (length > 0)
                {
                    response.send((char*) data, length);
                }
            },
            [] (int errorCode) -> void
            {
                std::cout << "An error has occured while reading" << "index.html (put the actual read file name here)" << ", error code: " << errorCode << std::endl;
            }
        );
    });
    
    Timer& timer = Timer::continousTimer([] (void* arguments) -> void {
        std::cout << "Tick" << std::endl;
    }, (struct timeval) { 1, 0 }, (void*) "Test");
    
    SocketMultiplexer& socketMultiplexer = SocketMultiplexer::instance();

    socketMultiplexer.getReadManager().manageSocket(serverSocket);
    
    while(1) {
        socketMultiplexer.tick();
    }
    
    return 0;
}
