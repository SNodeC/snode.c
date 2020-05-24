#ifndef INETADDRESS_H
#define INETADDRESS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <netinet/in.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class InetAddress
{
public:
    InetAddress(const InetAddress& ina);
    InetAddress(const std::string& ipOrHostname, uint16_t port);
    InetAddress(uint16_t port);
    InetAddress(struct sockaddr_in& addr);
    InetAddress();

    InetAddress& operator=(const InetAddress& ina);

    const struct sockaddr_in& getSockAddr() const;

private:
    struct sockaddr_in addr;
};

#endif // INETADDRESS_H
