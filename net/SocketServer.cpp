#include "SocketServer.h"

#include "Multiplexer.h"


void SocketServer::start() {
    Multiplexer::start();
}


void SocketServer::stop() {
    Multiplexer::stop();
}
