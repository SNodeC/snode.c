#ifndef SOCKETREADERBASE_H
#define SOCKETREADERBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Reader.h"


class SocketReaderBase : public Reader {
public:
    void readEvent() override;

protected:
    SocketReaderBase(const std::function<void(const char* junk, ssize_t n)>& readProcessor, const std::function<void(int errnum)>& onError)
        : readProcessor(readProcessor)
        , onError(onError) {
    }

    virtual ssize_t recv(char* junk, const ssize_t& junkSize) = 0;

private:
    std::function<void(const char* junk, ssize_t n)> readProcessor;

    std::function<void(int errnum)> onError;
};

#endif // SOCKETREADERBASE_H
