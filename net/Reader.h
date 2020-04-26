#ifndef READER_H
#define READER_H

#include <string>

#include "ManagedDescriptor.h"


class Reader : public ManagedDescriptor {
public:
    virtual void readEvent() = 0;
    
protected:
    Reader(int fd) : ManagedDescriptor(fd) {}
    
    virtual ~Reader() = default;
    
    std::string readPuffer;
};


#endif // READER_H
