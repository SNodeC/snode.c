#ifndef SOCKETCONNECTIONBASE_H
#define SOCKETCONNECTIONBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Multiplexer.h"
#include "SocketConnection.h"
#include "SocketServerBase.h"


class SocketServer;

// typedef std::function<void(SocketConnection* cs, const char* junk, ssize_t n)> ReadProcessorr;
// typedef std::function<void(SocketConnection* cs, int errnum)> OnError;

template <typename Reader, typename Writer>
class SocketConnectionBase
    : public SocketConnection
    , public Reader
    , public Writer {
public:
    void enqueue(const char* buffer, int size) override {
        Writer::writePuffer.append(buffer, size);
        Multiplexer::instance().getManagedWriter().add(this);
    }

    void end() override {
        Multiplexer::instance().getManagedReader().remove(this);
    }

    InetAddress& getRemoteAddress() override {
        return remoteAddress;
    }

    void setRemoteAddress(const InetAddress& remoteAddress) override {
        this->remoteAddress = remoteAddress;
    }

public:
    SocketConnectionBase(int csFd, SocketServerBase<SocketConnectionBase>* serverSocket,
                         const std::function<void(SocketConnectionBase* cs, const char* junk, ssize_t n)>& readProcessor,
                         const std::function<void(SocketConnectionBase* cs, int errnum)>& onReadError,
                         const std::function<void(SocketConnectionBase* cs, int errnum)>& onWriteError)
        : Reader(
              [&](const char* junk, ssize_t n) -> void {
                  readProcessor(this, junk, n);
              },
              [&](int errnum) -> void {
                  onReadError(this, errnum);
              })
        , Writer(
              [&](int errnum) -> void {
                onWriteError(this, errnum);
              })
        , serverSocket(serverSocket) {
        this->attachFd(csFd);
    }

private:
    void unmanaged() override {
        serverSocket->disconnect(this);
    }

    SocketServerBase<SocketConnectionBase>* serverSocket;

    InetAddress remoteAddress{};

public:
    using ReaderType = Reader;
    using WriterType = Writer;
};

#endif // SOCKETCONNECTIONBASE_H
