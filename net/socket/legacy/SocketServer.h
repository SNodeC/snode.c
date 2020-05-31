#ifndef SOCKETLEGACYSERVER_H
#define SOCKETLEGACYSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketServerBase.h"
#include "socket/legacy/SocketConnection.h"


namespace legacy {

    class SocketServer : public SocketServerBase<legacy::SocketConnection> {
    private:
        SocketServer(const std::function<void(::SocketConnection* cs)>& onConnect,
                     const std::function<void(::SocketConnection* cs)>& onDisconnect,
                     const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcesor,
                     const std::function<void(int errnum)>& onCsReadError, const std::function<void(int errnum)>& onCsWriteError);

    public:
        static SocketServer* instance(const std::function<void(::SocketConnection* cs)>& onConnect,
                                      const std::function<void(::SocketConnection* cs)>& onDisconnect,
                                      const std::function<void(::SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                                      const std::function<void(int errnum)>& onCsReadError,
                                      const std::function<void(int errnum)>& onCsWriteError);
    };

}; // namespace legacy

#endif // SOCKETLEGACYSERVER_H
