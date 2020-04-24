#ifndef READER_H
#define READER_H

#include "Manageable.h"

#include <string>

class Reader : public Manageable {
public:
    virtual ~Reader() = default;
    
//    virtual void readEvent() = 0;
    
    virtual void readEvent();
    
    std::string readPuffer;
    
};


#endif // READER_H
