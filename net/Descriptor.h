#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H


class Descriptor {
protected:
    Descriptor();

private:
    Descriptor& operator=(const Descriptor& descriptor) {
        return *this;
    }

public:
    virtual ~Descriptor();

    virtual void attachFd(int fd);

    int getFd() const;

private:
    int fd;
};

#endif // DESCRIPTOR_H
