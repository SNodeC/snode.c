#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


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

    int fd() const;

private:
    int _fd;
};

#endif // DESCRIPTOR_H
