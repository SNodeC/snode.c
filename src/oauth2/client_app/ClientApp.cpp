#include "express/legacy/in/WebApp.h"
#include "express/middleware/StaticMiddleware.h"
#include "express/middleware/VHost.h"

#include <iostream>

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);
    express::legacy::in::WebApp app{"OAuth2Client"};

    /*
    app.get("/oauth2", [] APPLICATION(req, res) {
        if (req.query("grant_type")) {
        }
        std::string tokenRequestUri{"http://localhost:8082/token"};
        tokenRequestUri += "?grant_type=authorization_code";
        tokenRequestUri += "&code=" + req.query("code");
        if (!req.query("state").empty()) {
            tokenRequestUri += "&state=" + req.query("state");
        }
        tokenRequestUri += "&client_id=911a821a-ea2d-11ec-8e2e-08002771075f";
        tokenRequestUri += "&redirect_uri=http://localhost:8081/oauth2";
        VLOG(0) << "Recieving auth code from auth server: " << req.query("code") << ", requesting token from " << tokenRequestUri;
        res.redirect(tokenRequestUri);
    });
    */

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
