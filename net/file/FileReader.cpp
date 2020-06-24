#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MFREADSIZE 16384

#include "FileReader.h"
#include "Multiplexer.h"


FileReader::FileReader(int fd, const std::function<void(char* data, int len)>& junkRead, const std::function<void(int err)>& onError)
    : Reader(/*onError*/)
    , junkRead(junkRead)
    , onError(onError)
    , stopped(false) {
    this->attachFd(fd);
}


FileReader* FileReader::read(const std::string& path, const std::function<void(char* data, int len)>& junkRead,
                             const std::function<void(int err)>& onError) {
    FileReader* fileReader = nullptr;

    int fd = ::open(path.c_str(), O_RDONLY);

    if (fd >= 0) {
        fileReader = new FileReader(fd, junkRead, onError);
        Multiplexer::instance().getManagedReader().add(fileReader);
    } else {
        onError(errno);
    }

    return fileReader;
}


void FileReader::stop() {
    if (!stopped) {
        Multiplexer::instance().getManagedReader().remove(this);
        this->onError(0);
        stopped = true;
    }
}


void FileReader::unmanaged() {
    delete this;
}


void FileReader::readEvent() {
    static char buffer[MFREADSIZE];

    int ret = ::read(this->getFd(), buffer, MFREADSIZE);

    if (!stopped) {
        if (ret > 0) {
            this->junkRead(buffer, ret);
        } else {
            stopped = true;
            Multiplexer::instance().getManagedReader().remove(this);
            this->onError(ret == 0 ? 0 : errno);
        }
    }
}
