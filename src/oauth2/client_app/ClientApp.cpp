#include "express/legacy/in/WebApp.h"
#include "express/middleware/StaticMiddleware.h"
#include "express/middleware/VHost.h"

#include <iostream>

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);
    express::legacy::in::WebApp app{"OAuth2Client"};

    app.get("/authCode", [] APPLICATION(req, res) {
        /*
        code=Yzk5ZDczMzRlNDEwY
        &grant_type=code
        &redirect_uri=https://example-app.com/cb
        &client_id=mRkZGFjM
        &client_secret=ZGVmMjMz
        &code_verifier=Th7UHJdLswIYQxwSg29DbK1a_d9o41uNMTRmuH0PM8zyoMAQcode=Yzk5ZDczMzRlNDEwY
        &grant_type=code
        &redirect_uri=https://example-app.com/cb
        &client_id=mRkZGFjM
        &client_secret=ZGVmMjMz
        &code_verifier=Th7UHJdLswIYQxwSg29DbK1a_d9o41uNMTRmuH0PM8zyoMAQ
        */

        std::string tokenRequestUri{"http://localhost:8082/token"};
        tokenRequestUri += "?grant_type=authorization_code";
        tokenRequestUri += "&code=" + req.query("code");
        if (req.query("redirect_uri").length() > 0) {
            tokenRequestUri += "&redirect_uri=" + req.query("redirect_uri");
        } else {
            tokenRequestUri += "&redirect_uri=http://localhost:8081/done";
        }
        tokenRequestUri += "&client_id=" + req.query("client_id");
        VLOG(0) << "Recieving auth code from auth server: " << req.query("code") << ", requestin token from " << tokenRequestUri;
        res.redirect(tokenRequestUri);
    });

    app.use(express::middleware::StaticMiddleware("/home/rathalin/projects/snode.c/src/oauth2/client_app/www"));

    app.listen(8081, [](const express::legacy::in::WebApp::SocketAddress socketAddress, int err) {
        if (err != 0) {
            std::cerr << "Failed to listen on port " << 8081 << std::endl;
        } else {
            std::cout << "OAuth2Client is listening on " << socketAddress.toString() << std::endl;
        }
    });

    return express::WebApp::start();
}
