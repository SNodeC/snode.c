#ifndef LEGACY_SOCKETSERVER_H
#define LEGACY_SOCKETSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketServer.h"
#include "socket/legacy/SocketConnection.h"


namespace legacy {

    class SocketServer : public ::SocketServer<legacy::SocketConnection> {};

}; // namespace legacy

#endif // LEGACY_SOCKETSERVER_H
