#include "SocketServer.h"

#include "Multiplexer.h"


void SocketServer::run() {
    Multiplexer::run();
}


void SocketServer::stop() {
    Multiplexer::stop();
}
