#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "WebApp.h"

#include "HTTPContext.h"
#include "socket/legacy/SocketServer.h"
#include "socket/tls/SocketServer.h"


WebApp::WebApp(const std::string& rootDir) {
    this->setRootDir(rootDir);
}


void WebApp::start(int argc, char* argv[]) {
    SocketServer::start(argc, argv);
}


void WebApp::stop() {
    SocketServer::stop();
}
