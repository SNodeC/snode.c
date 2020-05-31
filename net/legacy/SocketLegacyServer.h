#ifndef SOCKETLEGACYSERVER_H
#define SOCKETLEGACYSERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketLegacyConnection.h"
#include "SocketServerBase.h"


namespace legacy {

class SocketLegacyServer : public SocketServerBase<SocketLegacyConnection> {
private:
    SocketLegacyServer(const std::function<void(SocketConnection* cs)>& onConnect,
                       const std::function<void(SocketConnection* cs)>& onDisconnect,
                       const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& readProcesor,
                       const std::function<void(int errnum)>& onCsReadError, const std::function<void(int errnum)>& onCsWriteError);

public:
    static SocketLegacyServer* instance(const std::function<void(SocketConnection* cs)>& onConnect,
                                        const std::function<void(SocketConnection* cs)>& onDisconnect,
                                        const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                                        const std::function<void(int errnum)>& onCsReadError,
                                        const std::function<void(int errnum)>& onCsWriteError);
};

};

#endif // SOCKETLEGACYSERVER_H
