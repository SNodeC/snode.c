#include <iostream>

#include <string.h>
#include <time.h>

#include "Request.h"
#include "Response.h"
#include "ResponseCookie.h"
#include "ResponseCookieOptions.h"
#include "SingleshotTimer.h"
#include "ContinousTimer.h"
#include "HTTPServer.h"

#include "httputils.h"

#define BUILD_PATH "/home/student/nds/snode.c/build/html"

int simpleWebserver(int argc, char** argv) {
    HTTPServer& app = HTTPServer::instance(BUILD_PATH);
    
    Router router;
    
    router.get("/search", 
              [&] (const Request& req, const Response& res) -> void {
                  
                  std::string host = req.header("Host");
                  
                  //                std::cout << "Host: " << host << std::endl;
                  
                  std::string uri = req.originalUrl;
                  
                  //                std::cout << "RHeader: " << req.header("Accept") << std::endl;
                  
                  std::cout << "Uri: " << uri << std::endl;
                  
                  if (uri == "/") {
                      res.redirect("/index.html");
                  } else {
                      //                    std::cout << uri << std::endl;
                      
                      if (req.bodySize() != 0) {
                          std::cout << "Body: " << req.body << std::endl;
                      }
                      
                      res.sendFile(uri, [uri] (int ret) -> void {
                          if (ret != 0) {
                              std::cerr << uri << ": " << strerror(ret) << std::endl;
                          }
                      });
                  }
              });
    
//    app.serverRoot("/home/voc/projects/ServerVoc/build/html");
    
    app.get("/",
            [&] (const Request& req, const Response& res) -> void {
                
                std::string host = req.header("Host");
                
//                std::cout << "Host: " << host << std::endl;
                
                std::string uri = req.originalUrl;
                
//                std::cout << "RHeader: " << req.header("Accept") << std::endl;
                
                std::cout << "Uri: " << uri << std::endl;
                
                res.cookie("Test", "timerApp", ResponseCookieOptions().path("/images").maxAge(3600).httpOnly());
                
                if (uri == "/") {
                    res.redirect("/index.html");
                } else {
//                    std::cout << uri << std::endl;
                    
                    if (req.bodySize() != 0) {
                        std::cout << "Body: " << req.body << std::endl;
                    }
                    
                    res.sendFile(uri, [uri] (int ret) -> void {
                        if (ret != 0) {
                            std::cerr << uri << ": " << strerror(ret) << std::endl;
                        }
                    });
                }
            });
    app.get("/", router);
    
    /*
    app.get("/search", 
            [&] (const Request& req, const Response& res) -> void {
                
                std::string host = req.header("Host");
                
                //                std::cout << "Host: " << host << std::endl;
                
                std::string uri = req.originalUrl;
                
                //                std::cout << "RHeader: " << req.header("Accept") << std::endl;
                
                std::cout << "Uri: " << uri << std::endl;
                
                if (uri == "/") {
                    res.redirect("/index.html");
                } else {
                    //                    std::cout << uri << std::endl;
                    
                    if (req.bodySize() != 0) {
                        std::cout << "Body: " << req.body << std::endl;
                    }
                    
                    res.sendFile(uri, [uri] (int ret) -> void {
                        if (ret != 0) {
                            std::cerr << uri << ": " << strerror(ret) << std::endl;
                        }
                    });
                }
            });*/
    
    app.listen(8080, 
               [] (int err) -> void {
                   if (err != 0) {
                       perror("Listen");
                   } else {
                       std::cout << "snode.c listening on port 8080" << std::endl;
                   }
               }
    );
    
    app.destroy();
    
    return 0;
}


int main(int argc, char** argv) {
    return simpleWebserver(argc, argv);
}

