#ifndef REQUEST_H
#define REQUEST_H

#include <map>
#include <string>


class AcceptedSocket;

class Request {
public:
    Request(AcceptedSocket* as) : acceptedSocket(as) {
    }
    
    std::map<std::string, std::string>& header();
    
    bool isPost();
    bool isGet();
    bool isPut();
    
    const std::string requestURI();
    
    const char* body();
    int bodySize();
    
private:
    AcceptedSocket* acceptedSocket;
};

#endif // REQUEST_H
