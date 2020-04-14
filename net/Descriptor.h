#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

class Descriptor
{
public:
    Descriptor(int fd);
    virtual ~Descriptor();
    
    int getFd() const;
    
    /*
    virtual void readEvent() {};
    virtual void writeEvent() {};
    virtual void exceptionEvent() {};
    */
    
    void incManagedCount();
    void decManagedCount();
    
private:
    int fd;
    
    int managedCount;
};

#endif // DESCRIPTOR_H
