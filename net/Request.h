#ifndef REQUEST_H
#define REQUEST_H

#include <map>

class ConnectedSocket;

class Request
{
public:
    Request(ConnectedSocket* cs) : connectedSocket(cs) {
    }
    
    std::map<std::string, std::string>& header();
    
    const char* body();

    int bodySize();
    
private:
    ConnectedSocket* connectedSocket;
};

#endif // REQUEST_H
