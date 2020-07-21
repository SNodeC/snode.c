#ifndef HTTPCONTEXT_H
#define HTTPCONTEXT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPRequestParser.h"
#include "Request.h"
#include "Response.h"
#include "socket/SocketConnectionBase.h"


class FileReader;
class WebApp;

class HTTPContext {
public:
    HTTPContext(const WebApp& webApp, SocketConnectionBase* connectedSocket);

    void receiveData(const char* junk, size_t junkLen);
    void onReadError(int errnum);
    void onWriteError(int errnum);

private:
    void stopFileReader();

    void send(const char* buffer, int size);
    void send(const std::string& buffer);
    void sendFile(const std::string& file, const std::function<void(int ret)>& onError = nullptr);

    void sendHeader();

    void requestReady();

    void end();
    void prepareForRequest();

    void enqueue(const char* buf, size_t len);
    void enqueue(const std::string& str);

    SocketConnectionBase* connectedSocket;
    FileReader* fileReader = nullptr;
    const WebApp& webApp;

    bool headerSend = false;

    size_t sendLen = 0;

    bool requestInProgress = false;

    Request request{};
    Response response;

    HTTPRequestParser parser;

    friend class Response;
};

#endif // HTTPCONTEXT_H
