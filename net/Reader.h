#ifndef READER_H
#define READER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedDescriptor.h"


class Reader : public ManagedDescriptor {
public:
    ~Reader() override = default;

    virtual void readEvent() = 0;

protected:
    explicit Reader(const std::function<void(int errnum)>& onError = nullptr)
        : onError(onError) {
    }

    void setOnError(const std::function<void(int errnum)>& onError) {
        this->onError = onError;
    }

    void stash();
    void unstash();

    std::function<void(int errnum)> onError;
};


#endif // READER_H
