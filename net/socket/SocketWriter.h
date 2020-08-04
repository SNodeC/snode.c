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

#include "WriteEvent.h"

#define MAX_SEND_JUNKSIZE 16384

template <typename SocketImpl>
class SocketWriter
    : public WriteEvent
    , virtual public SocketImpl {
public:
    SocketWriter() = delete;

    explicit SocketWriter(const std::function<void(int errnum)>& onError)
        : onError(onError) {
    }

    ~SocketWriter() override {
        if (isManaged()) {
            WriteEvent::disable();
        }
    }

    void writeEvent() override {
        errno = 0;

        ssize_t ret = write(writeBuffer.data(), (writeBuffer.size() < MAX_SEND_JUNKSIZE) ? writeBuffer.size() : MAX_SEND_JUNKSIZE);

        if (ret >= 0) {
            writeBuffer.erase(writeBuffer.begin(), writeBuffer.begin() + ret);

            if (writeBuffer.empty()) {
                WriteEvent::disable();
            }
        } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            WriteEvent::disable();
            onError(errno);
        }
    }

    void enqueue(const char* buffer, size_t size) {
        writeBuffer.insert(writeBuffer.end(), buffer, buffer + size);
        WriteEvent::enable();
    }

protected:
    virtual ssize_t write(const char* junk, size_t junkSize) = 0;

    std::function<void(int errnum)> onError;

    std::vector<char> writeBuffer;
};

#endif // SOCKETWRITERBASE_H
