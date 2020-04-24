#ifndef SOCKET_H
#define SOCKET_H

#include "Descriptor.h"
#include "InetAddress.h"

class Socket : public Descriptor {
protected:
//    Socket();
/*    
private:
    Socket& operator=(const Socket& socket) {
        return *this;
    }
*/

public:
    Socket(int csFd);

    virtual ~Socket();
    
    int bind(InetAddress& localAddress);
    
    void setLocalAddress(const InetAddress& localAddress);
    
    InetAddress& getLocalAddress();

protected:
    InetAddress localAddress;
};

#endif // SOCKET_H
