#ifndef WRITER_H
#define WRITER_H

#include "Manageable.h"

#include <string>

class Writer : public Manageable {
public:
    virtual ~Writer() = default;
    
    virtual void writeEvent();
    
    std::string writePuffer;
};


#endif // WRITER_H
