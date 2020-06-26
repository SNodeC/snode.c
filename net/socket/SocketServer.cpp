#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketServer.h"

#include "Multiplexer.h"


void SocketServer::init(int argc, char* argv[]) {
    Multiplexer::init(argc, argv);
}


void SocketServer::start() {
    Multiplexer::start();
}


void SocketServer::stop() {
    Multiplexer::stop();
}
