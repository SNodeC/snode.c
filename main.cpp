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


int readLine(int fd, std::string& line) {
    int numRead = 0;
    int n = 0;
    char ch;
    static std::string newLine;
    
    do {
        n = recv(fd, &ch, 1, 0);
        if (n > 0) {
            numRead += n;
            if ( ch != '\n' && ch != '\r') {
                numRead += n;
                line += ch;
            }
        }
    } while(ch != '\n' && n == 1);
    
    if (n <= 0) {
        return n;
    }
    return numRead;
}

std::list<std::string>* readHeader(int fd) {
    std::list<std::string>* header = new std::list<std::string>();
    std::string line;
    int n;
    
    do {
        line.clear();
        n = readLine(fd, line);
        if (n > 0) {
            header->push_back(line);
        }
    } while (line.length() > 0);
    
    if (n <= 0) {
        delete header;
        return 0;
    }
    
    return header;
}

#define LAMBDACB(ret, name, args) std::function<ret (args)> name
#define LAMBDA(ret, args) [&] args -> ret

void readJunk(int fd, 
               LAMBDACB(void, JUNKcb, (const char* junk, int ret, int fd)),
               LAMBDACB(void, EOFcb, (int fd)),
               LAMBDACB(void, ERRcb, (int err, int fd))) {
    char junk[16];
    
    int ret = recv(fd, junk, 15, 0);
    
    if (ret > 0) {
        junk[ret] = 0;
        JUNKcb(junk, ret, fd);
    } else if (ret == 0) {
        EOFcb(fd);
    } else {
        ERRcb(ret, fd);
    }
}


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
            
            document = "<!DOCTYPE html>\n";
            document += "<html>\n<body>\n";
            document += "<h1>My First Heading</h1>\n";
            document += "<p>My first paragraph.</p>\n";
            document += "</body>\n</html>\n";
            
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
