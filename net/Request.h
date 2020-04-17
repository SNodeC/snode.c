#ifndef REQUEST_H
#define REQUEST_H

#include <map>

class AcceptedSocket;

class Request {
public:
    Request(AcceptedSocket* as) : acceptedSocket(as) {
    }
    
    std::map<std::string, std::string>& header();
    
    const char* body();

    int bodySize();
    
    bool isPost();
    bool isGet();
    bool isPut();
    
    const std::string requestURI();
    
private:
    AcceptedSocket* acceptedSocket;
};

#endif // REQUEST_H
