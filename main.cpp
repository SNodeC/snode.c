#include <iostream>

#include <time.h>

#include "Request.h"
#include "Response.h"
#include "SingleshotTimer.h"
#include "ContinousTimer.h"
#include "HTTPServer.h"

#include "httputils.h"

int main(int argc, char **argv) {
    Timer& tick = Timer::continousTimer(
        [] (const void* arg) -> void {
            static int i = 0;
            std::cout << (const char*) arg << " "<< i++ << std::endl;
        }, 
        (struct timeval) {0, 500000}, "Tick");
    
    Timer& tack = Timer::continousTimer(
        [] (const void* arg) -> void {
            static int i = 0;
            std::cout << (const char*) arg << " " << i++ << std::endl;
        }, 
        (struct timeval) {1, 100000}, "Tack");
        
    bool canceled = false;
    HTTPServer& app = HTTPServer::instance();
    
    app.serverRoot("/home/voc/projects/ServerVoc/build/html");
    
    app.get(
        [&] (const Request& req, const Response& res) -> void {
            std::string uri = req.requestURI();
            
            if (uri == "/") {
                uri = "/index.html";
            }
            
            std::cout << "RCookie: " << req.cookie("doxygen_width") << std::endl;
            std::cout << "RCookie: " << req.cookie("Test") << std::endl;
            
//            std::cout << "RHeader: " << req.header("Content-Length") << std::endl;
            std::cout << "RHeader: " << req.header("Accept") << std::endl;
            
            std::cout << "If-Modified: " << req.header("If-Modified-Since") << std::endl;

            std::cout << "RQuery: " << req.query("Hallo") << std::endl;
            
            res.cookie("Test", "me");
            
//            res.set("Connection", "close");
            res.sendFile(uri, [uri] (int ret) -> void {
                if (ret != 0) {
                    perror(uri.c_str());
//                    std::cout << "Error: " << ret << ", " << uri << std::endl;
                }
            });
            
            if (!canceled) {
                tack.cancel();
                canceled = true;
//                app.destroy();
            }
//            Multiplexer::stop();
        }
    );
    
    app.listen(8080, 
        [] (int err) -> void {
            if (err != 0) {
                perror("Listen");
            } else {
                std::cout << "Up and running" << std::endl;
            }
        }
    );

    app.destroy();
    
    return 0;
}

