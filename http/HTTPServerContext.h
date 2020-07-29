#ifndef HTTPCONTEXT_H
#define HTTPCONTEXT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPRequestParser.h"
#include "Request.h"
#include "Response.h"

class WebApp;
class SocketConnectionBase;

class HTTPServerContext {
public:
    HTTPServerContext(const WebApp& webApp, SocketConnectionBase* connectedSocket);

    void receiveRequestData(const char* junk, size_t junkLen);
    void onReadError(int errnum);

    void sendResponseData(const char* buf, size_t len);
    void onWriteError(int errnum);

    void requestReady();
    void requestCompleted();

    void terminateConnection();

private:
    SocketConnectionBase* connectedSocket;
    bool requestInProgress = false;

public:
    const WebApp& webApp;

    Request request;
    Response response;

private:
    HTTPRequestParser parser;
};

#endif // HTTPCONTEXT_H
