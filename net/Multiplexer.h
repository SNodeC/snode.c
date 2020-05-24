#ifndef SOCKETMULTIPLEXER_H
#define SOCKETMULTIPLEXER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <signal.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedReader.h"
#include "ManagedWriter.h"
#include "ExceptionManager.h"
#include "ManagedTimer.h"


class Multiplexer
{
private:
    Multiplexer() {
        signal(SIGPIPE, SIG_IGN);
    }

    ~Multiplexer() {}


public:
    static Multiplexer& instance() {
        return multiplexer;
    }

    ManagedReader& getReadManager() {
        return readManager;
    }

    ManagedWriter& getWriteManager() {
        return writeManager;
    }

    ExceptionManager& getExceptionManager() {
        return exceptionManager;
    }

    ManagedTimer& getTimerManager() {
        return managedTimer;
    }

    static void run();

    static void stop();

private:
    void tick();

    static Multiplexer multiplexer;

    ManagedReader readManager;
    ManagedWriter writeManager;
    ExceptionManager exceptionManager;
    ManagedTimer managedTimer;

    static bool running;
};


#endif // SOCKETMULTIPLEXER_H
