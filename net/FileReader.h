#ifndef FILEREADER_H
#define FILEREADER_H

#include <functional>
#include <string>

#include "Reader.h"
#include "Descriptor.h"


class FileReader : public Reader
{
protected:
    FileReader(int fd, const std::function<void (char* data, int len)>& junkRead, const std::function<void (int err)>& fn);
    
public:
    static void read(std::string path, const std::function<void (char* data, int len)>& junkRead, const std::function<void (int err)>& fn);
    
    virtual void readEvent();
    
protected:
    std::function<void (char* data, int len)> junkRead;
    std::function<void (int err)> fn;
    
};

#endif // FILEREADER_H
