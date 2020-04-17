#include <iostream>
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
    
    Server& app = Server::instance(8080);

    app.serverRoot("/home/voc/projects/html-pages/Static-Site-Samples/POHTML");

    app.get(
        [&] (Request& req, Response& res) -> void {
            std::string uri = req.requestURI();
            
            if (uri == "/") {
                uri = "/index.html";
            }
            
            res.sendFile(uri);
        }
    );


    while(1) {
        SocketMultiplexer::instance().tick();
    }

    return 0;
}
