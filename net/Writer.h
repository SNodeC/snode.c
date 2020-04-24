#ifndef WRITER_H
#define WRITER_H

#include <iostream>

#include "Manageable.h"

#include <string>

class Writer : public ManagedDescriptor {
public:
    Writer(int fd) : ManagedDescriptor(fd) {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }
    
    virtual ~Writer() = default;
    
    virtual void writeEvent() = 0;
    
    std::string writePuffer;
};


#endif // WRITER_H
