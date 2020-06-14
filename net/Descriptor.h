#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class Descriptor {
protected:
    Descriptor() = default;

public:
    Descriptor(const Descriptor& d) = delete;
    Descriptor& operator=(const Descriptor& descriptor) = delete;
    ~Descriptor();

    void attachFd(int fd);

    [[nodiscard]] int fd() const;

private:
    int _fd{-1};
};

#endif // DESCRIPTOR_H
