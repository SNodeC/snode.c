#ifndef EXCEPTION_H
#define EXCEPTION_H


class Exception {
public:
    virtual void exceptionEvent() = 0;
    
protected:
    Exception(): managed(false) {
    }
    
    virtual ~Exception() = default;

public:
    bool managed = false;
};


#endif // EXCEPTION_H
