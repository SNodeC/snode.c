#ifndef SOCKETCONNECTIONBASE_H
#define SOCKETCONNECTIONBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketConnectionBase.h"


template <typename Reader, typename Writer>
class SocketConnection
    : public SocketConnectionBase
    , public Reader
    , public Writer {
public:
    SocketConnection(int csFd, const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                     const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
                     const std::function<void(SocketConnection* cs, int errnum)>& onWriteError,
                     const std::function<void(SocketConnection* cs)>& onDisconnect)
        : Reader(
              [&](const char* junk, ssize_t n) -> void {
                  onRead(this, junk, n);
              },
              [&](int errnum) -> void {
                  onReadError(this, errnum);
              })
        , Writer([&](int errnum) -> void {
            onWriteError(this, errnum);
        })
        , onDisconnect(onDisconnect) {
        this->attachFd(csFd);
    }

    void enqueue(const char* buffer, int size) override {
        this->Writer::enqueue(buffer, size);
    }

    void end() override {
        Reader::stop();
    }

    InetAddress& getRemoteAddress() {
        return remoteAddress;
    }

    void setRemoteAddress(const InetAddress& remoteAddress) {
        this->remoteAddress = remoteAddress;
    }

private:
    void unmanaged() override {
        onDisconnect(this);
    }

    InetAddress remoteAddress{};
    std::function<void(SocketConnection* cs)> onDisconnect;

public:
    using ReaderType = Reader;
    using WriterType = Writer;
};

#endif // SOCKETCONNECTIONBASE_H
