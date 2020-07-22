#include "Server.h"

#include "ManagedServer.h" // for ManagedExceptions
#include "Multiplexer.h"


void Server::start() {
    Multiplexer::instance().getManagedServer().start(this);
}

void Server::stop() {
    Multiplexer::instance().getManagedServer().stop(this);
}
