#ifndef INETADDRESS_H
#define INETADDRESS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <netinet/in.h>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class InetAddress {
public:
    InetAddress() = default;
    InetAddress(const InetAddress& ina);
    explicit InetAddress(const std::string& ipOrHostname, uint16_t port);
    explicit InetAddress(uint16_t port);
    explicit InetAddress(const struct sockaddr_in& addr);

    ~InetAddress() = default;

    InetAddress& operator=(const InetAddress& ina);

    const struct sockaddr_in& getSockAddr() const;

private:
    struct sockaddr_in addr {};
};

#endif // INETADDRESS_H
