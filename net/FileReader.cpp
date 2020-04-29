#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "FileReader.h"
#include "Multiplexer.h"

#include <iostream>

FileReader::FileReader(int fd) : Reader(fd) {}


void FileReader::read(std::string path, std::function<void (char* data, int len)> junkRead, std::function<void (int err)> error) {
    int fd = open(path.c_str(), O_RDONLY);
    
    if (fd >= 0) {
        FileReader* reader = new FileReader(fd);
        reader->junkRead = junkRead;
        reader->error = error;
        Multiplexer::instance().getReadManager().manageSocket(reader);
    } else {
        error(errno);
    }
}


void FileReader::readEvent() {
    char puffer[4096];
    
    int ret = ::read(this->getFd(), puffer, 4096);
    
    if (ret > 0) {
        this->junkRead(puffer, ret);
    } else  if (ret == 0) {
        Multiplexer::instance().getReadManager().unmanageSocket(this);
    } else {
        this->error(errno);
        Multiplexer::instance().getReadManager().unmanageSocket(this);
    }
}
