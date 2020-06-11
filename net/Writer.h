#ifndef WRITER_H
#define WRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedDescriptor.h"


class Writer : public ManagedDescriptor {
public:
    virtual ~Writer() = default;

    virtual void writeEvent() = 0;

protected:
    Writer(const std::function<void(int errnum)>& onError)
        : ManagedDescriptor()
        , onError(onError) {
    }

    void stash();
    void unstash();

    std::string writePuffer;

    std::function<void(int errnum)> onError;
};


#endif // WRITER_H
