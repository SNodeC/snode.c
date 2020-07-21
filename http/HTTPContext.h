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

private:
    void requestReady();
    void prepareForRequest();
    void enqueue(const char* buf, size_t len);

    SocketConnectionBase* connectedSocket;
    const WebApp& webApp;
    bool requestInProgress = false;

    Request request{};
    Response response;

    HTTPRequestParser parser;

    friend class Response;
};

#endif // HTTPCONTEXT_H
