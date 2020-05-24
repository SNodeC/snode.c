#ifndef SOCKETMULTIPLEXER_H
#define SOCKETMULTIPLEXER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <signal.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ReadManager.h"
#include "WriteManager.h"
#include "ExceptionManager.h"
#include "TimerManager.h"


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

    ReadManager& getReadManager() {
        return readManager;
    }

    WriteManager& getWriteManager() {
        return writeManager;
    }

    ExceptionManager& getExceptionManager() {
        return exceptionManager;
    }

    TimerManager& getTimerManager() {
        return timerManager;
    }

    static void run();

    static void stop();

private:
    void tick();

    static Multiplexer multiplexer;

    ReadManager readManager;
    WriteManager writeManager;
    ExceptionManager exceptionManager;
    TimerManager timerManager;

    static bool running;
};


#endif // SOCKETMULTIPLEXER_H
