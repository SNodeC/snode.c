#ifndef LEGACY_SOCKETSERVER_H
#define LEGACY_SOCKETSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketConnection.h"
#include "socket/SocketServer.h"
#include "socket/legacy/SocketReader.h"
#include "socket/legacy/SocketWriter.h"


namespace legacy {

    using SocketConnection = SocketConnection<legacy::SocketReader, legacy::SocketWriter>;

    class SocketServer : public ::SocketServer<legacy::SocketConnection> {};

}; // namespace legacy

#endif // LEGACY_SOCKETSERVER_H
