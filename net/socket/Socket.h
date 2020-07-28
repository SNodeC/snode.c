#ifndef SOCKET_H
#define SOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "socket/InetAddress.h"


class Socket : virtual public Descriptor {
public:
    Socket(bool dontClose = false)
        : Descriptor(dontClose) {
    }

    Socket(const Socket&) = delete;

    virtual ~Socket();

    Socket& operator=(const Socket&) = delete;

    void open(const std::function<void(int errnum)>& onError, int flags = 0);
    void bind(const InetAddress& localAddress, const std::function<void(int errnum)>& onError);
    void reuseAddress(const std::function<void(int errnum)>& onError);

    InetAddress& getLocalAddress();
    void setLocalAddress(const InetAddress& localAddress);

protected:
    InetAddress localAddress{};
};

#endif // SOCKET_H
