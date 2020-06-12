#ifndef SOCKETBASE_H
#define SOCKETBASE_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "socket/InetAddress.h"


class Socket : virtual public Descriptor {
public:
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;

    virtual ~Socket();

    void bind(InetAddress& localAddress, const std::function<void(int errnum)>& onError);

    InetAddress& getLocalAddress();

    void setLocalAddress(const InetAddress& localAddress);

protected:
    Socket() = default;

    void open(const std::function<void(int errnum)>& onError);

    InetAddress localAddress;
};

#endif // SOCKETBASE_H
