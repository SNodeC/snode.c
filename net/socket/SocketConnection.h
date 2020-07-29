#ifndef SOCKETCONNECTIONBASE_H
#define SOCKETCONNECTIONBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "SocketConnectionBase.h"


template <typename SocketReader, typename SocketWriter>
class SocketConnection
    : public SocketConnectionBase
    , public SocketReader
    , public SocketWriter {
protected:
    SocketConnection(int csFd, const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                     const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
                     const std::function<void(SocketConnection* cs, int errnum)>& onWriteError,
                     const std::function<void(SocketConnection* cs)>& onDisconnect)
        : SocketReader(
              [&](const char* junk, ssize_t n) -> void {
                  onRead(this, junk, n);
              },
              [&](int errnum) -> void {
                  onReadError(this, errnum);
              })
        , SocketWriter([&](int errnum) -> void {
            onWriteError(this, errnum);
        })
        , onDisconnect(onDisconnect) {
        this->attachFd(csFd);
    }

public:
    static SocketConnection* create(int csFd, const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                                    const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
                                    const std::function<void(SocketConnection* cs, int errnum)>& onWriteError,
                                    const std::function<void(SocketConnection* cs)>& onDisconnect) {
        return new SocketConnection(csFd, onRead, onReadError, onWriteError, onDisconnect);
    }

    static SocketConnection* create(const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& onRead,
                                    const std::function<void(SocketConnection* cs, int errnum)>& onReadError,
                                    const std::function<void(SocketConnection* cs, int errnum)>& onWriteError,
                                    const std::function<void(SocketConnection* cs)>& onDisconnect) {
        return SocketConnection::create(0, onRead, onReadError, onWriteError, onDisconnect);
    }

private:
    void unmanaged() override {
        onDisconnect(this);
        delete this;
    }

public:
    void enqueue(const char* buffer, int size) override {
        this->SocketWriter::enqueue(buffer, size);
    }

    void enqueue(const std::string& data) override {
        this->enqueue(data.c_str(), data.size());
    }

    void end() override {
        SocketReader::stop();
    }

    InetAddress& getRemoteAddress() {
        return remoteAddress;
    }

    void setRemoteAddress(const InetAddress& remoteAddress) {
        this->remoteAddress = remoteAddress;
    }

private:
    InetAddress remoteAddress{};
    std::function<void(SocketConnection* cs)> onDisconnect;

public:
    using ReaderType = SocketReader;
    using WriterType = SocketWriter;
};

#endif // SOCKETCONNECTIONBASE_H
