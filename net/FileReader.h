#ifndef FILEREADER_H
#define FILEREADER_H

#include <functional>
#include <string>

#include "Reader.h"
#include "Descriptor.h"


class FileReader : public Reader
{
protected:
    FileReader(int fd);
    
public:
    static void read(std::string path, std::function<void (char* data, int len)> junkRead, std::function<void (int err)> error);
    
    virtual void readEvent();
    
protected:
    std::function<void (char* data, int len)> junkRead;
    std::function<void (int err)> error;
    
};

#endif // FILEREADER_H
