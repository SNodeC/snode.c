#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "legacy/WebApp.h"
#include "tls/WebApp.h"

#include <easylogging++.h>
#include <iostream>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MIDDLEWARE(req, res, next)                                                                                                         \
    [&]([[maybe_unused]] Request & (req), [[maybe_unused]] Response & (res),                                                               \
        [[maybe_unused]] const std::function<void(void)>&(next)) -> void
#define APPLICATION(req, res) [&]([[maybe_unused]] Request & (req), [[maybe_unused]] Response & (res)) -> void

#define CERTF "/home/voc/projects/ServerVoc/certs/calisto.home.vchrist.at_-_snode.c.pem"
#define KEYF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c.key.encrypted.pem"
#define KEYFPASS "snode.c"

#define SERVERROOT "/home/voc/projects/ServerVoc/build/html/"

int main(int argc, char** argv) {
    WebApp::init(argc, argv);

    Router router;
    router
        .use(
            "/",
            MIDDLEWARE(req, res, next) {
                res.set("Connection", "Keep-Alive");
                next();
            })
        .get(
            "/",
            APPLICATION(req, res) {
                VLOG(0) << "URL: " + req.originalUrl;
                if (req.originalUrl == "/") {
                    res.redirect(308, "/index.html");
                } else if (req.url == "/end") {
                    res.send("Bye, bye!\n");
                    WebApp::stop();
                } else {
                    res.sendFile(req.url, [&req](int ret) -> void {
                        if (ret != 0) {
                            PLOG(ERROR) << req.url;
                        }
                    });
                }
            })
        .get(
            "/search", APPLICATION(req, res) {
                VLOG(0) << "URL: " + req.url;
                res.sendFile(req.url, [&req](int ret) -> void {
                    if (ret != 0) {
                        PLOG(ERROR) << req.url;
                    }
                });
            });

    legacy::WebApp legacyApp(SERVERROOT);
    legacyApp.use("/", router);

    tls::WebApp tlsApp(SERVERROOT, CERTF, KEYF, KEYFPASS);
    tlsApp.use("/", router);

    tlsApp.listen(8088, [](int err) -> void {
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
