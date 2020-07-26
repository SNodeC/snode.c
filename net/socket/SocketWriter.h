#ifndef SOCKETWRITERBASE_H
#define SOCKETWRITERBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <stddef.h> // for size_t
#include <string>
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Writer.h"


class SocketWriter : public Writer {
public:
    void writeEvent() override;

protected:
    explicit SocketWriter(const std::function<void(int errnum)>& onError)
        : onError(onError) {
    }

    ~SocketWriter() {
        if (isManaged()) {
            Writer::stop();
        }
    }

    void enqueue(const char* buffer, int size);

    virtual ssize_t send(const char* junk, size_t junkSize) = 0;

    std::function<void(int errnum)> onError;

protected:
    std::string writePuffer;
};

#endif // SOCKETWRITERBASE_H
