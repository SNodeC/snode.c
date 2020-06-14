#ifndef LEGACY_SOCKETSERVER_H
#define LEGACY_SOCKETSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketServerBase.h"
#include "socket/legacy/SocketConnection.h"


namespace legacy {

    class SocketServer : public SocketServerBase<legacy::SocketConnection> {
    private:
        SocketServer(const std::function<void(legacy::SocketConnection* cs)>& onConnect,
                     const std::function<void(legacy::SocketConnection* cs)>& onDisconnect,
                     const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                     const std::function<void(::SocketConnection* cs, int errnum)>& onReadError,
                     const std::function<void(::SocketConnection* cs, int errnum)>& onWriteError);

    public:
        static SocketServer* instance(const std::function<void(legacy::SocketConnection* cs)>& onConnect,
                                      const std::function<void(legacy::SocketConnection* cs)>& onDisconnect,
                                      const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                                      const std::function<void(::SocketConnection* cs, int errnum)>& onReadError,
                                      const std::function<void(::SocketConnection* cs, int errnum)>& onWriteError);
    };

}; // namespace legacy

#endif // LEGACY_SOCKETSERVER_H
