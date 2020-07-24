#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class Descriptor {
protected:
    Descriptor(bool keepOpen = false);

public:
    Descriptor(const Descriptor& d) = delete;
    Descriptor& operator=(const Descriptor& descriptor) = delete;
    ~Descriptor();

    void attachFd(int fd);

    int getFd() const;

protected:
    bool keepOpen = false;

private:
    int fd{-1};
};

#endif // DESCRIPTOR_H
