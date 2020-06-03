#ifndef SOCKETBASE_H
#define SOCKETBASE_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "socket/InetAddress.h"


class Socket : virtual public Descriptor {
public:
    virtual ~Socket();

    void bind(InetAddress& localAddress, const std::function<void(int errnum)>& onError);

    InetAddress& getLocalAddress();

    void setLocalAddress(const InetAddress& localAddress);

protected:
    Socket();

    void open(const std::function<void(int errnum)>& onError);

    virtual ssize_t recv(void* buf, size_t len, int flags) = 0;
    virtual ssize_t send(const void* buf, size_t len, int flags) = 0;


    InetAddress localAddress;
};

#endif // SOCKETBASE_H
