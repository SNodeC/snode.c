#ifndef READER_H
#define READER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedDescriptor.h"


class Reader : public ManagedDescriptor {
public:
    virtual ~Reader() = default;

    virtual void readEvent() = 0;

protected:
    Reader(const std::function<void(int errnum)>& onError = 0)
        : ManagedDescriptor()
        , onError(onError) {
    }

    void setOnError(const std::function<void(int errnum)>& onError) {
        this->onError = onError;
    }

    std::function<void(int errnum)> onError;
};


#endif // READER_H
