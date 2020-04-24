#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

class Descriptor
{
protected:
    Descriptor();
    
private:
    Descriptor& operator=(const Descriptor& descriptor) {
        return *this;
    }
    
    Descriptor(const Descriptor& descriptor) {
        *this = descriptor;
    }
    
public:
    Descriptor(int fd);
    virtual ~Descriptor();
    
    int getFd() const;
    
private:
    int fd;
};

#endif // DESCRIPTOR_H
