#ifndef WRITER_H
#define WRITER_H

#include <functional>
#include <string>

#include "ManagedDescriptor.h"


class Writer : public ManagedDescriptor {
public:
    virtual void writeEvent() = 0;
    
protected:
    Writer(int fd, const std::function<void (int errnum)>& onError) : ManagedDescriptor(fd), onError(onError) {}
    
    virtual ~Writer() = default;
    
    std::string writePuffer;
    
    std::function<void (int errnum)> onError;
};


#endif // WRITER_H
