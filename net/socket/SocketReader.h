#ifndef SOCKETREADERBASE_H
#define SOCKETREADERBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <stddef.h>    // for size_t
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Reader.h"


class SocketReader : public Reader {
public:
    void readEvent() override;

protected:
    SocketReader(const std::function<void(const char* junk, ssize_t n)>& onRead, const std::function<void(int errnum)>& onError)
        : onRead(onRead)
        , onError(onError) {
        Reader::start();
    }

    virtual ssize_t recv(char* junk, size_t junkSize) = 0;

private:
    std::function<void(const char* junk, ssize_t n)> onRead;
    std::function<void(int errnum)> onError;
};

#endif // SOCKETREADERBASE_H
