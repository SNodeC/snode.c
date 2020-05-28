#include <iostream>
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "WebApp.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


int simpleWebserver(int argc, char** argv) {
    Router router;
    router.get("/", [&](const Request& req, const Response& res) -> void {
        std::cout << "URL: " << req.originalUrl << std::endl;
        std::cout << "Cookie: " << req.cookie("searchcookie") << std::endl;
        res.sendFile(req.originalUrl);
    });

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
        .get("/search", router);

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
