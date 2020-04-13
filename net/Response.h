#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>

class ConnectedSocket;

class Response
{
public:
    Response(ConnectedSocket* cs) {
        connectedSocket = cs;
    }
    
    void send(const std::string& text);
    
    void end();
    
private:
    ConnectedSocket* connectedSocket;
};


#endif // RESPONSE_H
