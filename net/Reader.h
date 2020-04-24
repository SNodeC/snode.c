#ifndef READER_H
#define READER_H

#include <iostream>

#include "Manageable.h"

#include <string>

class Reader : public ManagedDescriptor {
public:
    Reader(int fd) : ManagedDescriptor(fd) {
        std::cout << __PRETTY_FUNCTION__ << std::endl;
    }
    
    virtual ~Reader() = default;
    
    virtual void readEvent() = 0;
    
    std::string readPuffer;
};


#endif // READER_H
