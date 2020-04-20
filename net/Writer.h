#ifndef WRITER_H
#define WRITER_H


class Writer {
public:
    virtual void writeEvent() = 0;
    
protected:
    Writer(): managed(false) {
    }
    
    virtual ~Writer() = default;
    
private:
    bool managed;
};


#endif // WRITER_H
