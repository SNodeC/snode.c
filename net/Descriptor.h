#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

class Descriptor {
protected:
    explicit Descriptor(bool dontClose = false)
        : dontClose(dontClose) {
    }

public:
    Descriptor(const Descriptor& d) = delete;
    Descriptor& operator=(const Descriptor& descriptor) = delete;
    ~Descriptor();

    void attachFd(int fd);

    int getFd() const;

protected:
    bool dontClose = false;

private:
    int fd = -1;
};

#endif // DESCRIPTOR_H
