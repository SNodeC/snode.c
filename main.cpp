#include <iostream>
#include <signal.h>

#include "ServerSocket.h"
#include "Request.h"
#include "Response.h"

void init() {
    signal(SIGPIPE, SIG_IGN);
}

int main(int argc, char **argv) {
    init();
    
    Server& app = Server::instance(8080);

//    app.serverRoot("/home/voc/projects/html-pages/Static-Site-Samples/POHTML");
//    app.serverRoot("/home/voc/Downloads/greeny_661");
    app.serverRoot("/home/voc/projects/ServerVoc/sites/textured_stars");

    app.get(
        [] (Request& req, Response& res) -> void {
            std::string uri = req.requestURI();
            
            if (uri == "/") {
                uri = "/index.html";
            }
            res.set("Connection", "close");
            res.sendFile(uri);
        }
    );

    Server::run();

    return 0;
}
