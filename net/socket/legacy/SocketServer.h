#ifndef LEGACY_SOCKETSERVER_H
#define LEGACY_SOCKETSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketConnectionBase.h"
#include "socket/SocketServerBase.h"
#include "socket/legacy/SocketReader.h"
#include "socket/legacy/SocketWriter.h"


namespace legacy {

    class SocketServer : public SocketServerBase<SocketConnectionBase<legacy::SocketReader, legacy::SocketWriter>> {
    private:
        SocketServer(const std::function<void(SocketConnectionBase<legacy::SocketReader, legacy::SocketWriter>* cs)>& onConnect,
                     const std::function<void(SocketConnectionBase<legacy::SocketReader, legacy::SocketWriter>* cs)>& onDisconnect,
                     const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                     const std::function<void(::SocketConnection* cs, int errnum)>& onReadError,
                     const std::function<void(::SocketConnection* cs, int errnum)>& onWriteError);
    };

}; // namespace legacy

#endif // LEGACY_SOCKETSERVER_H
