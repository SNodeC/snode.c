#ifndef SOCKETWRITERBASE_H
#define SOCKETWRITERBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <cstddef> // for size_t
#include <functional>
#include <iostream>
#include <sys/types.h> // for ssize_t
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Writer.h"

#define MAX_SEND_JUNKSIZE 16384

template <typename SocketImpl>
class SocketWriter
    : public Writer
    , virtual public SocketImpl {
public:
    void writeEvent() override {
        errno = 0;

        ssize_t ret = send(writeBuffer.data(), (writeBuffer.size() < MAX_SEND_JUNKSIZE) ? writeBuffer.size() : MAX_SEND_JUNKSIZE);

        if (ret >= 0) {
            writeBuffer.erase(writeBuffer.begin(), writeBuffer.begin() + ret);

            if (writeBuffer.empty()) {
                Writer::stop();
            }
        } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            Writer::stop();
            onError(errno);
        }
    }

protected:
    SocketWriter() = delete;
    explicit SocketWriter(const std::function<void(int errnum)>& onError)
        : onError(onError) {
    }

    virtual ~SocketWriter() {
        if (isManaged()) {
            Writer::stop();
        }
    }

    void enqueue(const char* buffer, size_t size) {
        writeBuffer.insert(writeBuffer.end(), buffer, buffer + size);
        Writer::start();
    }

    virtual ssize_t send(const char* junk, size_t junkSize) = 0;

    std::function<void(int errnum)> onError;

protected:
    std::vector<char> writeBuffer;
};

#endif // SOCKETWRITERBASE_H
