#ifndef SOCKETWRITERBASE_H
#define SOCKETWRITERBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include <Writer.h>


class SocketWriter : public Writer {
public:
    void writeEvent() override;

protected:
    explicit SocketWriter(const std::function<void(int errnum)>& onError)
        : onError(onError) {
    }

    void enqueue(const char* buffer, int size);

    virtual ssize_t send(const char* junk, const ssize_t& junkSize) = 0;

    std::function<void(int errnum)> onError;

protected:
    std::string writePuffer;
};

#endif // SOCKETWRITERBASE_H
