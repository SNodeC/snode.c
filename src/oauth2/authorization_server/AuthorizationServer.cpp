#include <iostream>

#include "express/legacy/in/WebApp.h"
#include "express/Router.h"
#include "express/middleware/StaticMiddleware.h"
#include "express/middleware/VHost.h"
#include <vector>

int main(int argc, char *argv[]) {
    express::WebApp::init(argc, argv);
    express::legacy::in::WebApp app{"OAuth2AuthorizationServer"};

    app.get("/auth", [] APPLICATION(req, res) { // Replace with MIDDLEWARE(req, res, next)
        auto paramResponseType = req.query("response_type");
        VLOG(0) << paramResponseType;
        auto paramClientId = req.query("client_id");
        VLOG(0) << paramClientId;
        auto paramRedirectUri = req.query("redirect_uri");
        VLOG(0) << paramRedirectUri;
        if (paramResponseType == "code") {
            VLOG(0) << "Auth valid, redirecting to login";
            res.redirect("/login/index.html");
        } else {
            VLOG(0) << "Auth invalid";
            res.redirect(paramRedirectUri);
        }
    });
    
    app.post("/login", [] APPLICATION(req, res) {
        auto paramEmail = req.param("email");
        VLOG(0) << paramEmail;
        auto paramPassword = req.param("password");
        VLOG(0) << paramPassword;

        if (paramEmail == "myqemail.com" && paramPassword == "password") {
            // TODO
        } else {
            res.sendStatus(401);
        }

        res.status(500).send("TODO");
    });

    app.use(express::middleware::StaticMiddleware("/home/rathalin/projects/snode.c/src/oauth2/authorization_server/www"));

    app.listen(8082, [] (const express::legacy::in::WebApp::SocketAddress socketAddress, int err) {
        if (err != 0) {
            std::cerr << "Failed to listen on port " << 8082 << std::endl;
        } else {
            std::cout << "OAuth2AuthorizationServer is listening on " << socketAddress.toString() << std::endl;
        }
    });

    return express::WebApp::start();
}
