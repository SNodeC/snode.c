#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

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
    char puffer[4096];
    
    int ret = ::read(this->getFd(), puffer, 4096);
    
    if (ret > 0) {
        this->junkRead(puffer, ret);
    } else  if (ret == 0) {
        this->onError(0);
        Multiplexer::instance().getReadManager().unmanageSocket(this);
    } else {
        this->onError(errno);
        Multiplexer::instance().getReadManager().unmanageSocket(this);
    }
}
