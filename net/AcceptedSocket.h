#ifndef ACCEPTEDSOCKET_H
#define ACCEPTEDSOCKET_H

#include "ConnectedSocket.h"

class ServerSocket;

class AcceptedSocket : public ConnectedSocket
{
public:
    AcceptedSocket(int csFd, ServerSocket* ss);
    virtual void ready();
    
protected:
    ServerSocket* serverSocket;
    
    Request request;
    Response response;
    
    
friend class Response;
friend class Request;
};

#endif // ACCEPTEDSOCKET_H
