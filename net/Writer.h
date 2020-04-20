#ifndef WRITER_H
#define WRITER_H

#include "Manageable.h"

class Writer : public Manageable {
public:
    virtual ~Writer() = default;
    
    virtual void writeEvent() = 0;
};


#endif // WRITER_H
