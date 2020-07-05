#ifndef TLS_SOCKETCONNECTION_H
#define TLS_SOCKETCONNECTION_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketConnection.h"
#include "socket/legacy/SocketReader.h"
#include "socket/legacy/SocketWriter.h"


namespace legacy {

    using SocketConnection = ::SocketConnection<legacy::SocketReader, legacy::SocketWriter>;

} // namespace legacy

#endif // TLS_SOCKETCONNECTION_H

