#ifndef CLIENTSOCKET_H
#define CLIENTSOCKET_H

#include <functional>

#include "ConnectedSocket.h"
#include "InetAddress.h"


class ClientSocket {}; /* : public ConnectedSocket
{
private:
    ClientSocket(int csFd);
    
public:
    static ClientSocket* connect(const InetAddress& ina, std::function<void (Request& req)> callback);
    
    virtual void ready();
    
    virtual void push(const char* junk, int n);
    
protected:
    Request request;
    
private:
    std::function<void (Request& req)> callback;
};
*/
#endif // CLIENTSOCKET_H
