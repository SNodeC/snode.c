#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "Logger.h"
#include "legacy/WebApp.h"
#include "tls/WebApp.h"

#include <iostream>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


#define MIDDLEWARE(req, res, next) [&](const Request&(req), const Response&(res), const std::function<void(void)>&(next)) -> void
#define APPLICATION(req, res) [&](const Request&(req), const Response&(res)) -> void

#define CERTF "/home/voc/projects/ServerVoc/certs/calisto.home.vchrist.at_-_snode.c.pem"
#define KEYF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c.key.encrypted.pem"
#define KEYFPASS "snode.c"

Router route() {
    Router router;
    router.get(
        "/search", APPLICATION(req, res) {
            VLOG(0) << "URL: " + req.originalUrl;
            res.sendFile(req.originalUrl, [&req](int ret) -> void {
                if (ret != 0) {
                    PLOG(ERROR) << req.originalUrl;
                }
            });
        });


    return router;
}


tls::WebApp sslMain() {
    tls::WebApp sslApp("/home/voc/projects/ServerVoc/build/html/", CERTF, KEYF, KEYFPASS);
    sslApp
        .use(
            "/",
            MIDDLEWARE(req, res, next) {
                res.set("Connection", "Keep-Alive");
                next();
            })
        .get("/", route())
        .get("/", APPLICATION(req, res) {
                std::string uri = req.originalUrl;
                VLOG(0) << "URL: " + req.originalUrl;
                if (uri == "/") {
                    res.redirect("/index.html");
                } else if (uri == "/end") {
                    res.send("Bye, bye!\n");
                    WebApp::stop();
                } else {
                    res.sendFile(uri, [uri, &req](int ret) -> void {
                        if (ret != 0) {
                            PLOG(ERROR) << uri;
                        }
                    });
                }
            });

    return sslApp;
}


legacy::WebApp legacyMain() {
    legacy::WebApp legacyApp("/home/voc/projects/ServerVoc/build/html/");
    legacyApp.use("/", MIDDLEWARE(req, res, next) {
            if (req.originalUrl == "/end") {
                res.send("Bye, bye!\n");
                WebApp::stop();
            } else {
                VLOG(0) << "Redirect: https://calisto.home.vchrist.at:8088" + req.originalUrl;
                res.redirect("https://calisto.home.vchrist.at:8088" + req.originalUrl);
            }
        });

    return legacyApp;
}


int simpleWebserver() {
    legacy::WebApp legacyApp(legacyMain());

    tls::WebApp sslApp(sslMain());

    sslApp.listen(8088, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8088";
        } else {
            VLOG(0) << "snode.c listening on port 8088 for SSL/TLS connections";
        }
    });

    legacyApp.listen(8080, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8080";
        } else {
            VLOG(0) << "snode.c listening on port 8080 for legacy connections";
        }
    });

    WebApp::start();

    return 0;
}


int main(int argc, char** argv) {
    WebApp::init(argc, argv);

    return simpleWebserver();
}
