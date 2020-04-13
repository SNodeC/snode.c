#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

#include "ConnectedSocket.h"
#include "InetAddress.h"

class ClientSocket : public ConnectedSocket
{
    int connect(const InetAddress& ina);
};

#endif // CLIENTSOCKET_H
