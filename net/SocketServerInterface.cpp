#include "SocketServerInterface.h"
#include "Multiplexer.h"


void SocketServerInterface::run() {
    Multiplexer::run();
}


void SocketServerInterface::stop() {
    Multiplexer::stop();
}
