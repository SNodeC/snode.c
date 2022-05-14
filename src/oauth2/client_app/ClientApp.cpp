#include <iostream>

#include "express/legacy/in/WebApp.h"
#include "express/middleware/StaticMiddleware.h"

int main(int argc, char *argv[]) {
    express::WebApp::init(argc, argv);
    express::legacy::in::WebApp app{"OAuth2Client"};

    app.use(express::middleware::StaticMiddleware("/home/rathalin/projects/snode.c/src/oauth2/client_app/www"));

    app.listen(8080, [] (const express::legacy::in::WebApp::SocketAddress socketAddress, int err) {
        if (err != 0) {
            std::cerr << "Failed to listen on port " << 8080;
        } else {
            std::cout << "OAuth2Client is listening on " << socketAddress.toString() << ":8080" << std::endl;
        }
    });

    return express::WebApp::start();
}
