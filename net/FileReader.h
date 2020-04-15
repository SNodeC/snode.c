#ifndef FILEREADER_H
#define FILEREADER_H

#include <string>
#include <functional>

#include "Reader.h"
#include "Descriptor.h"


class FileReader : public Descriptor, public Reader
{
private:
    FileReader(int fd);
    
public:
    static void read(std::string path, std::function<void (unsigned char* data, int len)> junkRead, std::function<void (int err)> error);
    
    virtual void readEvent();
    
private:
    std::function<void (unsigned char* data, int len)> junkRead;
    std::function<void (int err)> error;
};

#endif // FILEREADER_H
