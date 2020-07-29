#ifndef TLS_SOCKETCONNECTION_H
#define TLS_SOCKETCONNECTION_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketConnection.h"
#include "socket/tls/SocketReader.h"
#include "socket/tls/SocketWriter.h"

namespace tls {

    using SocketConnection = ::SocketConnection<tls::SocketReader, tls::SocketWriter>;

} // namespace tls

#endif // TLS_SOCKETCONNECTION_H
