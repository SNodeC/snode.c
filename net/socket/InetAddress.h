#ifndef INETADDRESS_H
#define INETADDRESS_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <netinet/in.h>
#include <stdint.h> // for uint16_t
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class InetAddress {
public:
    InetAddress();
    InetAddress(const InetAddress& ina);
    explicit InetAddress(const std::string& ipOrHostname);
    explicit InetAddress(const std::string& ipOrHostname, uint16_t port);
    explicit InetAddress(in_port_t port);
    explicit InetAddress(const struct sockaddr_in& addr);

    in_port_t port();


    ~InetAddress() = default;

    InetAddress& operator=(const InetAddress& ina);

    const struct sockaddr_in& getSockAddr() const;

private:
    struct sockaddr_in addr {};
};

#endif // INETADDRESS_H
