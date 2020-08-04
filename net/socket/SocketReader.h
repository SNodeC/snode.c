#ifndef SOCKETREADERBASE_H
#define SOCKETREADERBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <cstddef> // for size_t
#include <functional>
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Reader.h"

#define MAX_READ_JUNKSIZE 16384

template <typename SocketImpl>
class SocketReader
    : public Reader
    , virtual public SocketImpl {
public:
    SocketReader() = delete;

    explicit SocketReader(const std::function<void(const char* junk, ssize_t n)>& onRead, const std::function<void(int errnum)>& onError)
        : onRead(onRead)
        , onError(onError) {
    }

    ~SocketReader() override {
        if (isManaged()) {
            Reader::stop();
        }
    }

    void readEvent() override {
        errno = 0;

        // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, hicpp-avoid-c-arrays, modernize-avoid-c-arrays)
        static char junk[MAX_READ_JUNKSIZE];

        ssize_t ret = recv(junk, MAX_READ_JUNKSIZE);

        if (ret > 0) {
            onRead(junk, ret);
        } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            Reader::stop();
            this->onError(ret == 0 ? 0 : errno);
        }
    }

protected:
    virtual ssize_t recv(char* junk, size_t junkSize) = 0;

private:
    std::function<void(const char* junk, ssize_t n)> onRead;
    std::function<void(int errnum)> onError;
};

#endif // SOCKETREADERBASE_H
