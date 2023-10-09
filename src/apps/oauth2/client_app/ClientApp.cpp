#include "express/legacy/in/WebApp.h"
#include "express/middleware/StaticMiddleware.h"
#include "log/Logger.h"

#include <iostream>

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);
    express::legacy::in::WebApp app("OAuth2Client");

    app.get("/oauth2", [] APPLICATION(req, res) {
        // if (req.query("grant_type")) {}
        if (!req.query("code").empty()) {
            res.sendFile("/home/rathalin/projects/snode.c/src/oauth2/client_app/vue-frontend-oauth2-client/dist/index.html",
                         [&req](int ret) -> void {
                             if (ret != 0) {
                                 PLOG(ERROR) << req.url;
                             }
                         });
            /*
            std::string tokenRequestUri{"http://localhost:8082/oauth2/token"};
            tokenRequestUri += "?grant_type=authorization_code";
            tokenRequestUri += "&code=" + req.query("code");
            if (!req.query("state").empty()) {
                tokenRequestUri += "&state=" + req.query("state");
            }
            tokenRequestUri += "&client_id=911a821a-ea2d-11ec-8e2e-08002771075f";
            tokenRequestUri += "&redirect_uri=http://localhost:8081/oauth2";
            VLOG(0) << "Recieving auth code from auth server: " << req.query("code") << ", requesting token from " << tokenRequestUri;
            res.redirect(tokenRequestUri);
            */
        }
    });

    app.use(express::middleware::StaticMiddleware("/home/rathalin/projects/snode.c/src/oauth2/client_app/vue-frontend-oauth2-client/dist"));

    app.listen(8081, [](const express::legacy::in::WebApp::SocketAddress socketAddress, int err) {
        if (err != 0) {
            std::cerr << "Failed to listen on port " << 8081 << std::endl;
        } else {
            std::cout << "OAuth2Client is listening on " << socketAddress.toString() << std::endl;
        }
    });

    return express::WebApp::start();
}
