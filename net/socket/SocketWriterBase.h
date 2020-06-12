#ifndef SOCKETWRITERBASE_H
#define SOCKETWRITERBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include <Writer.h>


class SocketWriterBase : public Writer {
public:
    virtual void writeEvent() override;

protected:
    SocketWriterBase(const std::function<void(int errnum)>& onError)
        : Writer(onError) {
    }

    virtual ssize_t send(const char* junk, const ssize_t& junkSize) = 0;
};

#endif // SOCKETWRITERBASE_H
