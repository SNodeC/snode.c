#include <iostream>

#include "HTTPServer.h"

int simpleWebserver(int argc, char** argv) {
    WebApp& app = WebApp::instance("/home/voc/projects/ServerVoc/build/html");
    app.get("/",
            [&] (const Request& req, const Response& res) -> void {
                std::string uri = req.originalUrl;
//                std::cout << "Connection: " << req.header("Connection") << std::endl;
//                std::cout << "URL: " << uri << std::endl;
//                std::cout << "Cookie: " << req.cookie("rootcookie") << std::endl;
//                res.cookie("searchcookie", "cookievalue", {{"Max-Age", "3600"}, {"Path", "/search"}});
//                res.clearCookie("rootcookie");
//                res.clearCookie("rootcookie");
//                res.clearCookie("searchcookie", {{"Path", "/search"}});
                if (uri == "/") {
                    res.redirect("/index.html");
                } else if (uri == "/end") {
                    app.stop();
                } else {
                    res.sendFile(uri);
                }
            });
    
    Router router;
    router.get("/search", 
               [&] (const Request& req, const Response& res) -> void {
                   std::cout << "URL: " << req.originalUrl << std::endl;
                   std::cout << "Cookie: " << req.cookie("searchcookie") << std::endl;
                   res.sendFile(req.originalUrl);
               });
    
    app.get("/", router);
    
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
