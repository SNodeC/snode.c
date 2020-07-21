#ifndef LEGACY_SOCKETREADER_H
#define LEGACY_SOCKETREADER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <stddef.h>    // for size_t
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "socket/SocketReader.h"
#include "socket/legacy/Socket.h"


namespace legacy {

    class SocketReader
        : public ::SocketReader
        , virtual public legacy::Socket {
    protected:
        SocketReader(const std::function<void(const char* junk, ssize_t n)>& onRead, const std::function<void(int errnum)>& onError)
            : ::SocketReader(onRead, onError) {
        }

    private:
        ssize_t recv(char* junk, size_t junkSize) override;
    };

}; // namespace legacy

#endif // LEGACY_SOCKETREADER_H
