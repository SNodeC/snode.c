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

#define SERVERROOT "/home/voc/projects/ServerVoc/build/html/"

int simpleWebserver() {
    Router router;
    router
        .use("/", MIDDLEWARE(req, res, next) {
            res.set("Connection", "Keep-Alive");
            next();
        })
        .get("/search", APPLICATION(req, res) {
            VLOG(0) << "URL: " + req.originalUrl;
            res.sendFile(req.originalUrl, [&req](int ret) -> void {
                if (ret != 0) {
                    PLOG(ERROR) << req.originalUrl;
                }
            });
        })
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

    legacy::WebApp legacyApp(SERVERROOT);
    legacyApp.use("/", router);

    tls::WebApp sslApp(SERVERROOT, CERTF, KEYF, KEYFPASS);
    sslApp.use("/", router);

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
