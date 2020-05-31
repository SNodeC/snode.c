#ifndef SOCKETWRITERBASE_H
#define SOCKETWRITERBASE_H

#include <Writer.h>


class SocketWriterBase : public Writer {
public:
    virtual void writeEvent();

protected:
    virtual ssize_t send(const char* junk, const ssize_t& junkSize) = 0;

    SocketWriterBase(const std::function<void(int errnum)>& onError)
        : Writer(onError) {
    }
};

#endif // SOCKETWRITERBASE_H
