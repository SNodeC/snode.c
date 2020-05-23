#include <iostream>

#include <string.h>
#include <time.h>

#include "Request.h"
#include "Response.h"
#include "ResponseCookie.h"
#include "SingleshotTimer.h"
#include "ContinousTimer.h"
#include "HTTPServer.h"

#include "httputils.h"

#define BUILD_PATH "/home/student/nds/snode.c/build/html"

int timerApp(int argc, char** argv) {
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
    HTTPServer& app = HTTPServer::instance(BUILD_PATH);
    
    app.get("/",
            [&] (const Request& req, const Response& res) -> void {
                std::string uri = req.originalUrl;
                
                if (uri == "/") {
                    uri = "/index.html";
                }
                
                std::cout << "RCookie: " << req.cookie("doxygen_width") << std::endl;
                std::cout << "RCookie: " << req.cookie("Test") << std::endl;
                
                //            std::cout << "RHeader: " << req.header("Content-Length") << std::endl;
                std::cout << "RHeader: " << req.header("Accept") << std::endl;
                
                std::cout << "If-Modified: " << req.header("If-Modified-Since") << std::endl;
                
                std::cout << "RQuery: " << req.query("Hallo") << std::endl;
                
                res.cookie("Test", "timerApp", ResponseCookieOptions("domain.com"));
                
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
                       std::cout << "snode.c listening on port 8080" << std::endl;
                   }
               }
    );
    
    app.destroy();
    
    return 0;
}


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
    
    app.get("/",
            [&] (const Request& req, const Response& res) -> void {
                res.cookie("Test", "simpleWebserver", {domain: "/"});
                
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
                
                
int testPost(int argc, char* argv[]) {
    HTTPServer& app = HTTPServer::instance(BUILD_PATH);
    
    app.get("/",
            [&] (const Request& req, const Response& res) -> void {
                res.send(
                    "<html>"\
                        "<head>"\
                            "<style>"\
                                "main {"\
                                    "min-height: 30em;"\
                                    "padding: 3em;"\
                                    "background-image: repeating-radial-gradient( circle at 0 0, #fff, #ddd 50px);"\
                                "}"\
                                "input[type=\"file\"] {"\
                                    "display: block;"\
                                    "margin: 2em;"\
                                    "padding: 2em;"\
                                    "border: 1px dotted;"\
                                "}"\
                            "</style>"\
                        "</head>"\
                        "<body>"\
                            "<h1>Datei-Upload mit input type=\"file\"</h1>"\
                            "<main>"\
                                "<h2>Schicken Sie uns was Schickes!</h2>"\
                                "<form method=\"post\" enctype=\"multipart/form-data\">"\
                                    "<label> Wählen Sie eine Textdatei (*.txt, *.html usw.) von Ihrem Rechner aus."\
                                        "<input name=\"datei\" type=\"file\" size=\"50\" accept=\"text/*\">"\
                                    "</label>"\
                                    "<button>… und ab geht die Post!</button>"\
                                "</form>"\
                            "</main>"\
                        "</body>"\
                    "</html>");
            });
    
    app.post("/", 
             [&] (const Request& req, const Response& res) -> void {
                 std::cout << "Content-Type: " << req.header("Content-Type") << std::endl;
                 std::cout << "Content-Length: " << req.header("Content-Length") << std::endl;
                 char* body = new char[std::stoi(req.header("Content-Length")) + 1];
                 memcpy(body, req.body, std::stoi(req.header("Content-Length")));
                 body[std::stoi(req.header("Content-Length"))] = 0;
                 
                 std::cout << "Body: " << std::endl;
                 std::cout << body << std::endl;
                 res.send(
                    "<html>"\
                        "<body>"\
                            "<h1>Thank you</h1>"\
                        "</body>"\
                    "</html>"
                );
             });
    
    app.listen(8080, 
               [] (int err) -> void {
                   if (err != 0) {
                       perror("Listen");
                   } else {
                       std::cout << "snode.c listening on port 8080" << std::endl;
                   }
               }
    );
    
    return 0;
}
                

int main(int argc, char** argv) {
    return simpleWebserver(argc, argv);
}

