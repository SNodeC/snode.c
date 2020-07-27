#ifndef SOCKETWRITERBASE_H
#define SOCKETWRITERBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <stddef.h> // for size_t
#include <string>
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Writer.h"


#define MAX_SEND_JUNKSIZE 4096

template <typename SocketImpl>
class SocketWriter
    : public Writer
    , virtual public SocketImpl {
public:
    void writeEvent() override {
        errno = 0;

        ssize_t ret = send(writePuffer.c_str(), (writePuffer.size() < MAX_SEND_JUNKSIZE) ? writePuffer.size() : MAX_SEND_JUNKSIZE);

        if (ret >= 0) {
            writePuffer.erase(0, ret);
            if (writePuffer.empty()) {
                Writer::stop();
            }
        } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            Writer::stop();
            onError(errno);
        }
    }

protected:
    explicit SocketWriter(const std::function<void(int errnum)>& onError)
        : onError(onError) {
    }

    ~SocketWriter() {
        if (isManaged()) {
            Writer::stop();
        }
    }

    void enqueue(const char* buffer, int size) {
        writePuffer.append(buffer, size);
        Writer::start();
    }

    virtual ssize_t send(const char* junk, size_t junkSize) = 0;

    std::function<void(int errnum)> onError;

protected:
    std::string writePuffer;
};

#endif // SOCKETWRITERBASE_H
