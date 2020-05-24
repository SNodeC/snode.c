#ifndef SOCKETWRITER_H
#define SOCKETWRITER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Writer.h"
#include "SocketLegacy.h"


class SocketLegacyWriter : public Writer, virtual public SocketLegacy {
public:
    void writeEvent();

protected:
    SocketLegacyWriter(const std::function<void (int errnum)>& onError) : Writer(onError) {}
};

#endif // SOCKETWRITER_H
