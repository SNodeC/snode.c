#include <string.h>
#include <netdb.h>

#include "InetAddress.h"


InetAddress::InetAddress(const std::string& ipOrHostname, uint16_t port) {
    struct hostent* he = gethostbyname(ipOrHostname.c_str());
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
}


InetAddress::InetAddress(uint16_t port) {
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
}


InetAddress::InetAddress(struct sockaddr_in& addr) {
    memcpy(&this->addr, &addr, sizeof(struct sockaddr_in));
}


InetAddress::InetAddress() {
}


InetAddress::InetAddress(const InetAddress& ina) {
    memcpy(&this->addr, &ina.addr, sizeof(struct sockaddr_in));
}


InetAddress& InetAddress::operator=(const InetAddress& ina) {
    memcpy(&this->addr, &ina.addr, sizeof(struct sockaddr_in));
    
    return *this;
}


struct sockaddr_in& InetAddress::getSockAddr() {
    return addr;
}
