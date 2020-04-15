#include "FileReader.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "SocketMultiplexer.h"

FileReader::FileReader(int fd) : Descriptor(fd) {
}


void FileReader::read(std::string path, std::function<void (unsigned char* data, int len)> junkRead, std::function<void (int err)> error) {
    int fd = open(path.c_str(), O_RDONLY);
    
    if (fd >= 0) {
        FileReader* reader = new FileReader(fd);
        reader->junkRead = junkRead;
        reader->error = error;
        SocketMultiplexer::instance().getReadManager().manageSocket(reader);
    } else {
        error(errno);
    }
}


void FileReader::readEvent() {
    unsigned char buffer[4096];
    
    int ret = ::read(this->getFd(), buffer, 4096);
    
    if (ret >= 0) {
        this->junkRead(buffer, ret);
        if (ret == 0) {
            SocketMultiplexer::instance().getReadManager().unmanageSocket(this);
        }
    } else {
        this->error(errno);
        SocketMultiplexer::instance().getReadManager().unmanageSocket(this);
    }
}
