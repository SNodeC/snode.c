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


class WebApp;

class HTTPContext {
public:
    HTTPContext(const WebApp& webApp, SocketConnectionBase* connectedSocket);

    void receiveData(const char* junk, size_t junkLen);
    void onReadError(int errnum);
    void onWriteError(int errnum);

    void requestReady();
    void enqueue(const char* buf, size_t len);
    void reset();
    void end();

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
