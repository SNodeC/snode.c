<<<<<<< HEAD
#include "FileReader.h"
=======
#ifndef DOXYGEN_SHOULD_SKIP_THIS
>>>>>>> MatthiasKoettritsch

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

<<<<<<< HEAD
#include "SocketMultiplexer.h"


FileReader::FileReader(int fd) : Descriptor(fd) {
}


void FileReader::read(std::string path, std::function<void (unsigned char* data, int len)> junkRead, std::function<void (int err)> error) {
    int fd = ::open(path.c_str(), O_RDONLY);
    
    if (fd >= 0) {
        FileReader* reader = new FileReader(fd);
        reader->junkRead = junkRead;
        reader->error = error;
        SocketMultiplexer::instance().getReadManager().manageSocket(reader);
    } else {
        error(errno);
    }
}


void FileReader::readEvent()
{
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
=======
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MFREADSIZE 16384

#include "FileReader.h"
#include "Multiplexer.h"


FileReader::FileReader (int fd, const std::function<void (char *data, int len)> &chunkRead, const std::function<void (int err)> &onError)
		: Descriptor(fd), Reader(onError), chunkRead(chunkRead)
{}


FileReader *
FileReader::read (std::string path, const std::function<void (char *data, int len)> &chunkRead, const std::function<void (int err)> &onError)
{
	FileReader *fileReader = 0;
	
	int fd = open(path.c_str(), O_RDONLY);
	
	if (fd >= 0)
	{
		fileReader = new FileReader(fd, chunkRead, onError);
		Multiplexer::instance().getReadManager().manageSocket(fileReader);
	} else
	{
		onError(errno);
	}
	
	return fileReader;
}


void FileReader::stop ()
{
	Multiplexer::instance().getReadManager().unmanageSocket(this);
}


void FileReader::readEvent ()
{
	char puffer[MFREADSIZE];
	
	int ret = ::read(this->getFd(), puffer, MFREADSIZE);
	
	if (ret > 0)
	{
		this->chunkRead(puffer, ret);
	} else if (ret == 0)
	{
		Multiplexer::instance().getReadManager().unmanageSocket(this);
		this->onError(0);
	} else
	{
		Multiplexer::instance().getReadManager().unmanageSocket(this);
		this->onError(errno);
	}
>>>>>>> MatthiasKoettritsch
}
