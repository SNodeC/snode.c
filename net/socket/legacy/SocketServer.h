#ifndef LEGACY_SOCKETSERVER_H
#define LEGACY_SOCKETSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketConnectionBase.h"
#include "socket/SocketServerBase.h"
#include "socket/legacy/SocketReader.h"
#include "socket/legacy/SocketWriter.h"


namespace legacy {

    using SocketConnection = SocketConnectionBase<legacy::SocketReader, legacy::SocketWriter>;

    class SocketServer : public SocketServerBase<legacy::SocketConnection> {};

}; // namespace legacy

#endif // LEGACY_SOCKETSERVER_H
