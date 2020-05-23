#ifndef SSLCONNECTEDSOCKET_H
#define SSLCONNECTEDSOCKET_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "InetAddress.h"
#include "SSLSocketWriter.h"
#include "SSLSocketReader.h"


class FileReader;
template<typename T> class BaseServerSocket;

class SSLConnectedSocket : public SSLSocketReader, public SSLSocketWriter
{
public:
    SSLConnectedSocket(int csFd, 
                    BaseServerSocket<SSLConnectedSocket>* ss, 
                    const std::function<void (SSLConnectedSocket* cs, const char*  junk, ssize_t n)>& readProcessor,
                    const std::function<void (int errnum)>& onReadError,
                    const std::function<void (int errnum)>& onWriteError
                   );
    
    virtual ~SSLConnectedSocket();
    
    void setContext(void* context) {
        this->context = context;
    }
    
    void* getContext() {
        return this->context;
    }
    
    virtual void send(const char* puffer, int size);
    virtual void send(const std::string& junk);
    virtual void sendFile(const std::string& file, const std::function<void (int ret)>& onError);
    void end();
    
    InetAddress& getRemoteAddress();
    void setRemoteAddress(const InetAddress& remoteAddress);

protected:
    BaseServerSocket<SSLConnectedSocket>* serverSocket;
    void* context;
    
    InetAddress remoteAddress;
    
private:
    FileReader* fileReader;
};

#endif // SSLCONNECTEDSOCKET_H
