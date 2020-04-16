#include <iostream>
#include <magic.h>
#include <string.h>
#include <signal.h>

#include "ServerSocket.h"
#include "SocketMultiplexer.h"
#include "Request.h"
#include "Response.h"

void init() {
    signal(SIGPIPE, SIG_IGN);
}

int main(int argc, char **argv) {
    init();
    
    magic_t magic = magic_open(MAGIC_MIME);
    if (magic_load(magic, NULL) != 0) {
        std::cout << "cannot load magic database - " << magic_error(magic) << std::endl;
        magic_close(magic);
        return 1;
    }
    std::cout << "hier: " << magic_file(magic, "./index.html") << std::endl;
    
    
    ServerSocket* serverSocket = ServerSocket::instance(8080);
    
    serverSocket->serverRoot("/home/voc/projects/html-pages/Static-Site-Samples/POHTML");
    
    serverSocket->get([&] (Request& req, Response& res) -> void {
        /*
        std::map<std::string, std::string>& header = req.header();
        std::map<std::string, std::string>::iterator it;
        
        for (it = header.begin(); it != header.end(); ++it) {
            std::cout << (*it).first << ": " << (*it).second << std::endl;
        }
        */
//        res.header();
        std::string uri = req.requestURI();
        
        std::cout << "URI: " << uri << std::endl;
        if (uri == "/") {
            uri = "/index.html";
        }
        std::cout << "URI: " << uri << std::endl;
        
        std::cout << "Content-Type: " << magic_file(magic, ("/home/voc/projects/html-pages/Static-Site-Samples/POHTML" + uri).c_str()) << std::endl;
    
        res.contentType(magic_file(magic, ("/home/voc/projects/html-pages/Static-Site-Samples/POHTML" + uri).c_str()));
        res.sendFile(uri);
    });
    
    
    while(1) {
        SocketMultiplexer::instance().tick();
    }
    
    return 0;
}
