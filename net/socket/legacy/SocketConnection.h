#ifndef LEGACY_SOCKETCONNECTION_H
#define LEGACY_SOCKETCONNECTION_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketConnectionBase.h"
#include "socket/legacy/SocketReader.h"
#include "socket/legacy/SocketWriter.h"


namespace legacy {

    class SocketConnection : public SocketConnectionBase<legacy::SocketReader, legacy::SocketWriter> {
    public:
        SocketConnection(int csFd, ::SocketServer* serverSocket,
                         const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                         const std::function<void(::SocketConnection* cs, int errnum)>& onReadError,
                         const std::function<void(::SocketConnection* cs, int errnum)>& onWriteError);
    };

}; // namespace legacy

#endif // LEGACY_SOCKETCONNECTION_H
