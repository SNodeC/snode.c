#ifndef SOCKETMULTIPLEXER_H
#define SOCKETMULTIPLEXER_H

#include <signal.h>

#include "ReadManager.h"
#include "WriteManager.h"
#include "ExceptionManager.h"
#include "TimerManager.h"


class Multiplexer
{
private:
    Multiplexer() {
        struct sigaction sh;
        struct sigaction osh;

        sh.sa_handler = SIG_IGN;
        sh.sa_flags = SA_RESTART;

        if (sigaction(SIGPIPE, &sh, &osh) < 0) {
            exit (-1);
        }
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
