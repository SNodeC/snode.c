#ifndef READER_H
#define READER_H


class Reader {
public:
    virtual void readEvent() = 0;
    
protected:
    Reader(): managed(false) {
    }
    
    virtual ~Reader() = default;

public:
    bool managed = false;
};


#endif // READER_H
