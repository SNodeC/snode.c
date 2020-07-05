#ifndef HTTPCONTEXT_H
#define HTTPCONTEXT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "Response.h"
#include "socket/SocketConnectionBase.h"


class FileReader;
class WebApp;

class HTTPContext {
private:
    enum struct requeststates { REQUEST, HEADER, BODY, ERROR } requestState{requeststates::REQUEST};

    enum struct linestate { READ, EOL } lineState{linestate::READ};

public:
    HTTPContext(const WebApp& webApp, SocketConnectionBase* connectedSocket);

    void receiveData(const char* junk, ssize_t junkLen);
    void onReadError(int errnum);
    void onWriteError(int errnum);

private:
    void stopFileReader();

    void send(const char* buffer, int size);
    void send(const std::string& buffer);
    void sendFile(const std::string& file, const std::function<void(int ret)>& onError = nullptr);

    void sendHeader();

    void parseRequest(const char* junk, ssize_t junkLen, const std::function<void(std::string&)>& lineRead,
                      const std::function<void(const char* bodyJunk, int junkLength)>& bodyRead);
    void parseRequestLine(const std::string& line);
    void parseCookie(const std::string& value);
    void addRequestLine(const std::string& line);

    void headerRead();
    void bodyRead();

    void requestReady();

    void end();
    void prepareForRequest();

    void enqueue(const char* buf, size_t len);
    void enqueue(const std::string& str);

    SocketConnectionBase* connectedSocket;
    FileReader* fileReader{nullptr};
    const WebApp& webApp;

    std::string headerLine;
    int bodyPointer{0};

    bool headerSend{false};

    int contentLength{0};
    int sendLen{0};

    bool requestInProgress{false};

    Request request{};
    Response response;

    friend class Response;
};

#endif // HTTPCONTEXT_H
