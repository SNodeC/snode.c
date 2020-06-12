#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketServer.h"

#include "Multiplexer.h"


void SocketServer::start(int argc, char** argv) {
    Multiplexer::start(argc, argv);
}


void SocketServer::stop() {
    Multiplexer::stop();
}
