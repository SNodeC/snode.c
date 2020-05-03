#ifndef READER_H
#define READER_H

#include <functional>
#include <string>

#include "ManagedDescriptor.h"


class Reader : public ManagedDescriptor {
public:
    virtual void readEvent() = 0;
    
protected:
    Reader(int fd, const std::function<void (int errnum)>& onError) : ManagedDescriptor(fd), onError(onError) {}
    
    void setOnError(const std::function<void (int errnum)>& onError) {
        this->onError = onError;
    }
    
    virtual ~Reader() = default;
    
    std::function<void (int errnum)> onError;
};


#endif // READER_H
