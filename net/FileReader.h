#ifndef FILEREADER_H
#define FILEREADER_H

#include <functional>
#include <string>

#include "Reader.h"
#include "Descriptor.h"


class FileReader : public Reader
{
protected:
    FileReader(int fd, const std::function<void (char* data, int len)>& junkRead, const std::function<void (int err)>& onError);
    
public:
    static FileReader* read(std::string path, const std::function<void (char* data, int len)>& junkRead, const std::function<void (int err)>& onError);
    
    void stop();
    
    virtual void readEvent();
    
protected:
    std::function<void (char* data, int len)> junkRead;
    
};

#endif // FILEREADER_H
