#include <iostream>
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "WebApp.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

Router route() {
    Router router;
    router.use("/1", [&](const Request& req, const Response& res, const std::function<void(void)>& next) -> void {
        std::cout << "URL: " << req.originalUrl << std::endl;
        std::cout << "Cookie 1: " << req.cookie("searchcookie") << std::endl;
        
        next();
    });
    
    router.get("/search", [&](const Request& req, const Response& res) -> void {
        std::cout << "URL: " << req.originalUrl << std::endl;
        std::cout << "Cookie 2: " << req.cookie("searchcookie") << std::endl;
        res.sendFile(req.originalUrl);
    });
    
    Router r;
    r.use("/3",  [&](const Request& req, const Response& res, const std::function<void(void)>& next) -> void {
        std::cout << "URL: " << req.originalUrl << std::endl;
        std::cout << "Cookie 1: " << req.cookie("searchcookie") << std::endl;
        
        next();
    });
    
    router.use("/4", r);
    
    std::cout << "Bevore return " << __PRETTY_FUNCTION__ << std::endl;
    
    return router;
}


Router rrr() {
    Router router(route());
    
    std::cout << "Bevore return " << __PRETTY_FUNCTION__ << std::endl;
    return router;
}


int simpleWebserver(int argc, char** argv) {
    std::cout << "Bevore rrr() " << __PRETTY_FUNCTION__ << std::endl;
    Router router1 = rrr();
    
    std::cout << "Bevore Router route " << __PRETTY_FUNCTION__ << std::endl;
    Router router;
    
    std::cout << "Bevore router = router1 " << __PRETTY_FUNCTION__ << std::endl;
    router = router1;

    WebApp& legacyApp = WebApp::instance("/home/voc/projects/ServerVoc/build/html/");
    legacyApp.use("/", [](const Request& req, const Response& res, const std::function<void(void)>& next) {
        std::cout << "Redirect: "
                  << "https://localhost:8088" + req.originalUrl << std::endl;
        res.redirect("https://localhost:8088" + req.originalUrl);
    });

    WebApp& sslApp = WebApp::instance("/home/voc/projects/ServerVoc/build/html/");
    sslApp
        .use("/",
             [&](const Request& req, const Response& res, const std::function<void(void)>& next) {
                 res.set("Connection", "Keep-Alive");
                 next();
             })
        .get("/",
             [&](const Request& req, const Response& res) -> void {
                 std::string uri = req.originalUrl;
                 std::cout << "URL: " << uri << std::endl;
                 std::cout << "Cookie: " << req.cookie("rootcookie") << std::endl;
                 res.cookie("searchcookie", "cookievalue", {{"Max-Age", "3600"}, {"Path", "/search"}});
                 //                res.clearCookie("rootcookie");
                 //                res.clearCookie("rootcookie");
                 //                res.clearCookie("searchcookie", {{"Path", "/search"}});
                 if (uri == "/") {
                     res.redirect("/index.html");
                 } else if (uri == "/end") {
                     WebApp::stop();
                 } else {
                     res.sendFile(uri);
                 }
             })
        .get("/", router);

#define CERTF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c.pem"
#define KEYF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c.key.encrypted.pem"

    sslApp.sslListen(8088, CERTF, KEYF, "snode.c", [&legacyApp](int err) -> void {
        if (err != 0) {
            perror("Listen");
        } else {
            std::cout << "snode.c listening on port 8088" << std::endl;
            legacyApp.listen(8080, [](int err) -> void {
                if (err != 0) {
                    perror("Listen");
                } else {
                    std::cout << "snode.c listening on port 8080" << std::endl;
                }
            });
        }
    });

    legacyApp.destroy();
    sslApp.destroy();

    return 0;
}


int main(int argc, char** argv) {
    return simpleWebserver(argc, argv);
}
