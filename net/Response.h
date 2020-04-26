#ifndef RESPONSE_H
#define RESPONSE_H

#include <map>
#include <string>


class AcceptedSocket;

class Response {
public:
    Response(AcceptedSocket* as);
    
    void status(int status);
    
    void send(const std::string& text);
    
    void send(const char* puffer, int n);
    
    void sendFile(const std::string& file);
    
    void end();
    
    void set(const std::string& field, const std::string& value);
    
    void append(const std::string& field, const std::string& value);
    
private:
    AcceptedSocket* acceptedSocket;
};


#endif // RESPONSE_H
