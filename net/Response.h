#ifndef RESPONSE_H
#define RESPONSE_H

#include <string>
#include <map>

class AcceptedSocket;

class Response
{
public:
    Response(AcceptedSocket* as);
    
    void contentType(const std::string& contentType) {
        headerMap["Content-Type"] = contentType;
    }
    
    void contentLength(int length) {
        headerMap["Content-Length"] = std::to_string(length);
    }
    
    void status(int status) {
        this->stat = status;
    }
    
    void send(const std::string& text);
    
    void send(const char* buffer, int n);
    
    void sendFile(const std::string& file);
    
    void end();
    
protected:
    void sendHeader();
    
private:
    AcceptedSocket* acceptedSocket;
    
    std::map<std::string, std::string> headerMap;
    int stat;
    bool headerSent;
    
    std::map<int, std::string> statusCode;
};


#endif // RESPONSE_H
