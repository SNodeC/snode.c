#ifndef READER_H
#define READER_H

#include "Manageable.h"

class Reader : public Manageable {
public:
    virtual ~Reader() = default;
    
    virtual void readEvent() = 0;
};


#endif // READER_H
