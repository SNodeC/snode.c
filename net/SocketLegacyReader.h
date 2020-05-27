#ifndef SOCKETREADER_H
#define SOCKETREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Reader.h"
#include "SocketLegacy.h"


class SocketConnection;

class SocketLegacyReader
    : public Reader
    , virtual public SocketLegacy {
public:
    void readEvent();

protected:
    SocketLegacyReader()
        : readProcessor(0) {
    }

    SocketLegacyReader(const std::function<void(SocketConnection* cs, const char* junk, ssize_t n)>& readProcessor,
                       const std::function<void(int errnum)>& onError)
        : SocketLegacy()
        , Reader(onError)
        , readProcessor(readProcessor) {
    }

    std::function<void(SocketConnection* cs, const char* junk, ssize_t n)> readProcessor;
};

#endif // SOCKETREADER_H
