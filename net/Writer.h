#ifndef WRITER_H
#define WRITER_H

#include <string>

#include "ManagedDescriptor.h"


class Writer : public ManagedDescriptor {
public:
    virtual void writeEvent() = 0;
    
protected:
    Writer(int fd) : ManagedDescriptor(fd) {}
    
    virtual ~Writer() = default;
    
    std::string writePuffer;
};


#endif // WRITER_H
