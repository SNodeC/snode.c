#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Multiplexer.h"
#include "WebApp.h"


bool WebApp::initialized{false};

WebApp::WebApp(const std::string& rootDir) {
    if (!initialized) {
        std::cerr << "ERROR: WebApp not initialized. Use WebApp::init(argc, argv) before creating a concrete WebApp object" << std::endl;
        exit(1);
    }
    this->setRootDir(rootDir);
}


void WebApp::init(int argc, char* argv[]) {
    Multiplexer::init(argc, argv);
    WebApp::initialized = true;
}


void WebApp::start() {
    Multiplexer::start();
}


void WebApp::stop() {
    Multiplexer::stop();
}
