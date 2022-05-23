#include <iostream>

#include "express/legacy/in/WebApp.h"
#include "express/middleware/StaticMiddleware.h"
#include "express/middleware/VHost.h"

int main(int argc, char *argv[]) {
    express::WebApp::init(argc, argv);
    express::legacy::in::WebApp app{"OAuth2Client"};

    app.use(express::middleware::StaticMiddleware("/home/rathalin/projects/snode.c/src/oauth2/client_app/www"));

    app.listen(8081, [] (const express::legacy::in::WebApp::SocketAddress socketAddress, int err) {
        if (err != 0) {
            std::cerr << "Failed to listen on port " << 8081 << std::endl;
        } else {
            std::cout << "OAuth2Client is listening on " << socketAddress.toString() << std::endl;
        }
    });

    return express::WebApp::start();
}
