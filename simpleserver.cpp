#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iostream>
#include <unistd.h>

#include "WebApp.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MIDDLEWARE(req, res, next) [&](const Request& (req), const Response& (res), const std::function<void(void)>& (next)) -> void
#define APPLICATION(req, res) [&](const Request& (req), const Response& (res)) -> void

Router route() {
    Router router;
    router.use(
        "/", MIDDLEWARE(req, res, next) {
            std::cout << "Middleware 1 " << req.originalUrl << std::endl;
            std::cout << "Cookie 1: " << req.cookie("searchcookie") << std::endl;

            next();
            
            req.setAttribute<std::string>("Hallo");
            req.setAttribute<std::string>("World", "Key1");
            req.setAttribute<int>(3);

            std::string hello = req.getAttribute<std::string>();
            std::string world = req.getAttribute<std::string>("Key1");
            int i = req.getAttribute<int>();
            float f = req.getAttribute<float>();

            std::cout << "String: --------- " << hello << std::endl;
            std::cout << "String: --------- " << world << std::endl;
            std::cout << "Int: --------- " << i << std::endl;
            std::cout << "Float: ---------- " << f << std::endl;
        });

    Router r;
    r.use(MIDDLEWARE(req, res, next) {
        std::cout << "Middleware 2 " << req.originalUrl << std::endl;
        std::cout << "Cookie 2: " << req.cookie("searchcookie") << std::endl;

        next();
    });

    router.use("/", r);

    router.get(
        "/search", APPLICATION(req, res) {
            std::cout << "Search" << std::endl;
            std::cout << "URL: " << req.originalUrl << std::endl;
            std::cout << "Cookie 3: " << req.cookie("searchcookie") << std::endl;
            res.sendFile(req.originalUrl);
        });


    return router;
}


WebApp sslMain() {
    WebApp sslApp("/home/voc/projects/ServerVoc/build/html/");
    sslApp
        .use(
            "/",
            MIDDLEWARE(req, res, next) {
                res.set("Connection", "Keep-Alive");
                next();
            })
        .get("/", route())
        .get(
            "/", APPLICATION(req, res) {
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
                    res.send("Bye, bye!");
                    WebApp::stop();
                } else {
                    res.sendFile(uri, [uri](int ret) -> void {
                        if (ret != 0) {
                            perror(uri.c_str());
                        }
                    });
                }
            });

    return sslApp;
}


WebApp legacyMain() {
    WebApp legacyApp("/home/voc/projects/ServerVoc/build/html/");
    legacyApp.use(
        "/", MIDDLEWARE(req, res, next) {
            if (req.originalUrl == "/end") {
                    res.send("Bye, bye!");
                    WebApp::stop();
            } else {
                std::cout << "Redirect: "
                          << "https://calisto.home.vchrist.at:8088" + req.originalUrl << std::endl;
                res.redirect("https://calisto.home.vchrist.at:8088" + req.originalUrl);
            }
        });

    return legacyApp;
}


int simpleWebserver(int argc, char** argv) {
    WebApp legacyApp(legacyMain());

    WebApp sslApp(sslMain());

#define CERTF "/home/voc/projects/ServerVoc/certs/calisto.home.vchrist.at_-_snode.c.pem"
#define KEYF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c.key.encrypted.pem"
#define KEYFPASS "snode.c"

    sslApp.sslListen(8088, CERTF, KEYF, KEYFPASS, [](int err) -> void {
        if (err != 0) {
            perror("Listen");
        } else {
            std::cout << "snode.c listening on port 8088 for SSL/TLS connections" << std::endl;
        }
    });

    legacyApp.listen(8080, [](int err) -> void {
        if (err != 0) {
            perror("Listen");
        } else {
            std::cout << "snode.c listening on port 8080 for legacy connections" << std::endl;
        }
    });

//    daemon(0, 0);

    WebApp::start(argc, argv);

    return 0;
}


int main(int argc, char** argv) {
    return simpleWebserver(argc, argv);
}
