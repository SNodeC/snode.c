#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "FileReader.h"
#include "Multiplexer.h"


FileReader::FileReader(int fd, const std::function<void (char* data, int len)>& junkRead, const std::function<void (int err)>& fn) : Reader(fd), junkRead(junkRead), fn(fn) {}


void FileReader::read(std::string path, const std::function<void (char* data, int len)>& junkRead, const std::function<void (int err)>& fn) {
    int fd = open(path.c_str(), O_RDONLY);
    
    if (fd >= 0) {
        FileReader* reader = new FileReader(fd, junkRead, fn);
        Multiplexer::instance().getReadManager().manageSocket(reader);
    } else {
        fn(errno);
    }
}


void FileReader::readEvent() {
    char puffer[4096];
    
    int ret = ::read(this->getFd(), puffer, 4096);
    
    if (ret > 0) {
        this->junkRead(puffer, ret);
    } else  if (ret == 0) {
        this->fn(0);
        Multiplexer::instance().getReadManager().unmanageSocket(this);
    } else {
        this->fn(errno);
        Multiplexer::instance().getReadManager().unmanageSocket(this);
    }
}
