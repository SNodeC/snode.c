#ifndef SOCKETCONNECTION_H
#define SOCKETCONNECTION_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "AttributeInjector.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class SocketConnectionBase : public utils::SingleAttributeInjector {
public:
    SocketConnectionBase(const SocketConnectionBase&) = delete;
    SocketConnectionBase& operator=(const SocketConnectionBase&) = delete;

    virtual ~SocketConnectionBase() = default;

    virtual void enqueue(const char* buffer, int size) = 0;

    virtual void end() = 0;

protected:
    SocketConnectionBase() = default;
};

#endif // SOCKETCONNECTION_H
