#ifndef SERVER_H
#define SERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedDescriptor.h"


class Server : public ManagedDescriptor {
public:
    ~Server() override = default;

    virtual void acceptEvent() = 0;

protected:
    void start();
    void stop();
};


#endif // SERVER_H
