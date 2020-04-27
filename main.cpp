#include <iostream>
#include <signal.h>

#include "ServerSocket.h"
#include "Request.h"
#include "Response.h"
#include "SingleshotTimer.h"
#include "ContinousTimer.h"
#include "HTTPServer.h"

void init() {
    signal(SIGPIPE, SIG_IGN);
}

int main(int argc, char **argv) {
    init();
    
    HTTPServer& app = HTTPServer::instance(8080);

    app.serverRoot("/home/voc/projects/ServerVoc/doc/html");

    Timer& tick = Timer::continousTimer(
        [] (const void* arg) -> void {
            static int i = 0;
            std::cout << (const char*) arg << " " << i++ << std::endl;
        }, 
        (struct timeval) {0, 500000}, "Tick");
    
    Timer& tack = Timer::continousTimer(
        [] (const void* arg) -> void {
            static int i = 0;
            std::cout << (const char*) arg << " " << i++ << std::endl;
        }, 
        (struct timeval) {1, 100000}, "Tack");
    bool canceled = false;
    
    app.get(
        [&] (const Request& req, const Response& res) -> void {
            std::string uri = req.requestURI();
            
            if (uri == "/") {
                uri = "/index.html";
            }
//            res.set("Connection", "close");
            res.sendFile(uri);
            
            if (!canceled) {
                tack.cancel();
                canceled = true;
            }
        }
    );

    
    ServerSocket::run();

    return 0;
}

