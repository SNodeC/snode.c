#ifndef FILEREADER_H
#define FILEREADER_H

#include <string>
#include <functional>

#include "Descriptor.h"
#include "Reader.h"

/**
 * @todo write docs
 */
class FileReader : public Descriptor, public Reader
{
public:
    static void read(std::string path, std::function<void (unsigned char* data, int length)> chunkRead, std::function<void (int errorCode)> error);
    
    virtual void readEvent();

private:
    FileReader(int fileDescriptor);
    
    
    std::function<void (unsigned char* data, int length)> chunkRead;
    std::function<void (int errorCode)> error;
};

#endif // FILEREADER_H
