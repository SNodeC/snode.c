#include <iostream>

#include "WebApp.h"

int simpleWebserver(int argc, char** argv) {
    WebApp& app = WebApp::instance("/home/voc/projects/ServerVoc/build/html");
    
    app.use("/",
            [&] (const Request& req, const Response& res, const std::function<void (void)>& next) {
                res.set("Connection", "Keep-Alive");
                next();
            });
                
    
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
//        res.set({{"Connection", "Keep-Alive"}});
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
//        res.set({{"Connection", "Keep-Alive"}});
        std::cout << "URL: " << req.originalUrl << std::endl;
        std::cout << "Cookie: " << req.cookie("searchcookie") << std::endl;
        res.sendFile(req.originalUrl);
    });

    app.get("/", router);
    
    #define CERTF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c.pem"
    #define KEYF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c.key.pem"
    
    app.listen(8080,
    [&] (int err) -> void {
        if (err != 0) {
            perror("Listen");
        } else {
            std::cout << "snode.c listening on port 8080" << std::endl;
            app.sslListen(8088, CERTF, KEYF, "password",
            [] (int err) -> void {
                if (err != 0) {
                    perror("Listen");
                } else {
                    std::cout << "snode.c listening on port 8088" << std::endl;
                }
            }
                         );
        }
    }
              );


    app.destroy();

    return 0;
}


int main(int argc, char** argv) {
    return simpleWebserver(argc, argv);
}
