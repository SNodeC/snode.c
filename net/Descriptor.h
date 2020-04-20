#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

class Descriptor
{
public:
    Descriptor(int fd);
    virtual ~Descriptor();
    
    int getFd() const;
    
private:
    int fd;
};

#endif // DESCRIPTOR_H
