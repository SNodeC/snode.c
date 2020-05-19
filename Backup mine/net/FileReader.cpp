#include "FileReader.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "SocketMultiplexer.h"


FileReader::FileReader(int fileDescriptor) : Descriptor(fileDescriptor) { }


void FileReader::read(std::string path, std::function<void (unsigned char *, int)> chunkRead, std::function<void (int)> error)
{
    int fileDescriptor = ::open(path.c_str(), O_RDONLY);
    
    if (fileDescriptor >= 0)
    {
        FileReader* reader = new FileReader(fileDescriptor);
        
        reader->chunkRead = chunkRead;
        reader->error = error;
        
        SocketMultiplexer::instance().getReadManager().manageSocket(reader);
    }
    else
    {
        error(errno);
    }
}


void FileReader::readEvent()
{
    int maxReadSize = 4096;
    unsigned char buffer[maxReadSize];
    
    int returnValue = ::read(this->getFd(), buffer, maxReadSize);
    
    if (returnValue >= 0)
    {
        this->chunkRead(buffer, returnValue);
        
        if (returnValue == 0)
        {
            SocketMultiplexer::instance().getReadManager().unmanageSocket(this);
        }
    }
    else
    {
        this->error(errno);
        SocketMultiplexer::instance().getReadManager().unmanageSocket(this);
    }
}
