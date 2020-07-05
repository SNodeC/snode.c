#ifndef SOCKETCONNECTION_H
#define SOCKETCONNECTION_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "AttributeInjector.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */


class SocketConnection : public utils::AttributeInjector {
public:
    SocketConnection(const SocketConnection&) = delete;
    SocketConnection& operator=(const SocketConnection&) = delete;

    virtual ~SocketConnection() = default;

    virtual void enqueue(const char* buffer, int size) = 0;

    virtual void end() = 0;

protected:
    SocketConnection() = default;
};

#endif // SOCKETCONNECTION_H
