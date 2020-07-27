#ifndef SOCKETREADERBASE_H
#define SOCKETREADERBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <errno.h>
#include <functional>
#include <stddef.h>    // for size_t
#include <sys/types.h> // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Reader.h"


#define MAX_READ_JUNKSIZE 16384

template <typename SocketImpl>
class SocketReader
    : public Reader
    , virtual public SocketImpl {
public:
    void readEvent() override {
        errno = 0;

        static char junk[MAX_READ_JUNKSIZE];

        ssize_t ret = recv(junk, MAX_READ_JUNKSIZE);

        if (ret > 0) {
            onRead(junk, ret);
        } else {
            if (errno != EAGAIN) {
                Reader::stop();
            }
            this->onError(ret == 0 ? 0 : errno);
        };
    }

protected:
    explicit SocketReader(const std::function<void(const char* junk, ssize_t n)>& onRead, const std::function<void(int errnum)>& onError)
        : onRead(onRead)
        , onError(onError) {
        Reader::start();
    }

    ~SocketReader() {
        if (isManaged()) {
            Reader::stop();
        }
    }

    virtual ssize_t recv(char* junk, size_t junkSize) = 0;

private:
    std::function<void(const char* junk, ssize_t n)> onRead;
    std::function<void(int errnum)> onError;
};

#endif // SOCKETREADERBASE_H
