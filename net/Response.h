#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>

class AcceptedSocket;

class Response
{
public:
    Response(AcceptedSocket* as) : acceptedSocket(as) {
    }
    
    void send(const std::string& text);
    
    void end();
    
private:
    AcceptedSocket* acceptedSocket;
};


#endif // RESPONSE_H
