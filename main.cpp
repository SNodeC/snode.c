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

/*
 std::string s = "scott>=tiger>=mushroom";
 s td::string delimiter = ">="*;
 
 size_t pos = 0;
 std::string token;
 while ((pos = s.find(delimiter)) != std::string::npos) {
     token = s.substr(0, pos);
     std::cout << token << std::endl;
     s.erase(0, pos + delimiter.length());
     }
     std::cout << s << std::endl;
*/


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


int Main(int argc, char **argv) {
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


int main(int argc, char **argv) {
    Main(argc, argv);
/*    
    int serverSocket;
    
    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    
    if (serverSocket < 0) {
        std::cerr << "Socket not created" << std::endl;
        exit(-1);
    }
    
    int sockopt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(serverSocket, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        std::cerr << "Socket not bound" << std::endl;
        exit(-1);
    }
    
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Socket cound not be switched to the server state" << std::endl;
        exit(-1);
    }
    
    struct sockaddr_in clientAddress;
    socklen_t addrlen = sizeof(clientAddress);
    
    fd_set readfds;
    
    FD_ZERO(&readfds);
    
    FD_SET(serverSocket, &readfds);
    
    struct timeval timeout;
    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    
    std::list<int> sl;
    
    while(true) {
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        
        int maxFd = serverSocket;
        
        std::list<int>::iterator it;
        
        for (it = sl.begin(); it != sl.end(); ++it) {
            FD_SET(*it, &readfds);
            
            if (maxFd < *it) {
                maxFd = *it;
            }
        }
        
        timeout.tv_sec = 10;
        
        int ret = select(maxFd + 1, &readfds, 0, 0, &timeout);
        
        if (FD_ISSET(serverSocket, &readfds)) {
            int connectedSocket = accept(serverSocket, (struct sockaddr *) &clientAddress, &addrlen);
            if (connectedSocket >= 0) {
                std::cout << "Add descriptor " << connectedSocket << std::endl;
                sl.push_back(connectedSocket);
            }
        }

        it = sl.begin();
        while(it != sl.end()) {
            if (FD_ISSET(*it, &readfds)) {
                readJunk(*it,
                          LAMBDA(void, (const char* junk, int n, int fd)) {
                              static std::string myJunk;
                              for (int i = 0; i < n; i++) {
                                  if (junk[i] != '\r' && junk[i] != '\n') {
                                      myJunk += junk[i];
                                  }
                                  if (junk[i] == '\n') {
                                      std::cout << fd << ": " << myJunk << std::endl;
                                      myJunk.clear();
                                  }
                              }
                              ++it;
                          },
                          LAMBDA(void, (int fd)) {
                              std::cout << "EOF: " << fd << std::endl;
                              ::close(fd);
                              it = sl.erase(it);
                          },
                          LAMBDA(void, (int err, int fd)) {
                              std::cout << "ERR " << err << std::endl;
                              ::close(fd);
                              it = sl.erase(it);
                          });
            } else {
                ++it;
            }
        }
    }
    */
}
