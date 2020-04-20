#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>
#include <map>

class AcceptedSocket;

class Response {
public:
    Response(AcceptedSocket* as);
    
    void contentType(const std::string& contentType);
    
    void contentLength(int length);
    
    void status(int status);
    
    void send(const std::string& text);
    
    void send(const char* buffer, int n);
    
    void sendFile(const std::string& file);
    
    void end();
    
    void sendHeader();
    
private:
    AcceptedSocket* acceptedSocket;
    
    std::map<std::string, std::string> responseHeader;
    int responseStatus;
    bool headerSent;
};


#endif // RESPONSE_H
