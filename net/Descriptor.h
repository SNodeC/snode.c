#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H


class Descriptor {
protected:
    Descriptor();

    Descriptor(int fd);

private:
    Descriptor& operator=(const Descriptor& descriptor) {
        return *this;
    }


public:
    virtual ~Descriptor();

    int getFd() const;

    virtual void attach(int fd) {
        this->fd = fd;
    }

private:
    int fd;
};

#endif // DESCRIPTOR_H
