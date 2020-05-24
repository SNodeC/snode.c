#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MFREADSIZE 16384

#include "FileReader.h"
#include "Multiplexer.h"


FileReader::FileReader(int fd, const std::function<void (char* data, int len)>& junkRead, const std::function<void (int err)>& onError) : Descriptor(fd), Reader(onError), junkRead(junkRead) {}


FileReader* FileReader::read(std::string path, const std::function<void (char* data, int len)>& junkRead, const std::function<void (int err)>& onError) {
    FileReader* fileReader = 0;

    int fd = open(path.c_str(), O_RDONLY);

    if (fd >= 0) {
        fileReader = new FileReader(fd, junkRead, onError);
        Multiplexer::instance().getReadManager().manageSocket(fileReader);
    } else {
        onError(errno);
    }

    return fileReader;
}


void FileReader::stop() {
    Multiplexer::instance().getReadManager().unmanageSocket(this);
}


void FileReader::readEvent() {
    char puffer[MFREADSIZE];

    int ret = ::read(this->getFd(), puffer, MFREADSIZE);

    if (ret > 0) {
        this->junkRead(puffer, ret);
    } else if (ret == 0) {
        this->stop();
        this->onError(0);
    } else {
        this->stop();
        this->onError(errno);
    }
}
