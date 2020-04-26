#ifndef ACCEPTEDSOCKET_H
#define ACCEPTEDSOCKET_H

#include <map>
#include <functional>

#include "ConnectedSocket.h"
#include "Request.h"
#include "Response.h"


class ServerSocket;

class AcceptedSocket : public ConnectedSocket
{
public:
    AcceptedSocket(int csFd, ServerSocket* ss, const InetAddress& remoteAddress, const InetAddress& localAccress);
    ~AcceptedSocket();
    
    virtual void send(const char* puffer, int size);
    virtual void send(const std::string& data);
    virtual void sendJunk(const char* puffer, int size);
    virtual void sendFile(const std::string& file);
    
    virtual void end();
    
private:
    virtual void readEvent();
    virtual void writeEvent();
    
    void readLine(std::function<void (std::string)> lineRead);
    void addRequestHeader(std::string& line);
    void requestReady();    
    void sendHeader();

    Request request;
    Response response;
    
    std::string requestLine;
    std::map<std::string, std::string> requestHeader;
    std::map<std::string, std::string> responseHeader;
    
    
    char* bodyData;
    int bodyLength;
    
    enum states {
        REQUEST,
        HEADER,
        BODY,
        ERROR
    } state;

    
    int bodyPointer;
    std::string line;
    
    bool headerSent;
    int responseStatus;
    
    enum linestate {
        READ,
        EOL
    } linestate;
    
    std::string hl;
    
    friend class Response;
    friend class Request;
};

#endif // ACCEPTEDSOCKET_H

