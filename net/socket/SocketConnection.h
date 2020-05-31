#ifndef SOCKETCONNECTIONINTERFACE_H
#define SOCKETCONNECTIONINTERFACE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/InetAddress.h"

class SocketConnection {
public:
    virtual ~SocketConnection() = default;

    virtual void setContext(void* context) = 0;
    virtual void* getContext() = 0;

    virtual void send(const char* puffer, int size) = 0;
    virtual void send(const std::string& junk) = 0;
    virtual void end() = 0;

    virtual InetAddress& getRemoteAddress() = 0;
    virtual void setRemoteAddress(const InetAddress& remoteAddress) = 0;

protected:
    SocketConnection() = default;
};

#endif // SOCKETCONNECTIONINTERFACE_H
