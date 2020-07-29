#ifndef LEGACY_SOCKETCONNECTION_H
#define LEGACY_SOCKETCONNECTION_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketConnection.h"
#include "socket/legacy/SocketReader.h"
#include "socket/legacy/SocketWriter.h"

namespace legacy {

    using SocketConnection = ::SocketConnection<legacy::SocketReader, legacy::SocketWriter>;

} // namespace legacy

#endif // LEGACY_SOCKETCONNECTION_H
