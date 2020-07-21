#ifndef SERVER_H
#define SERVER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedDescriptor.h"


class Server : public ManagedDescriptor {
public:
    ~Server() override = default;

    virtual void acceptEvent() = 0;

protected:
    void start() override;
    void stop() override;
};


#endif // SERVER_H
