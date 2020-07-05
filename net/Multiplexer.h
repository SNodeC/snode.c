#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "ManagedExceptions.h"
#include "ManagedReader.h"
#include "ManagedTimer.h"
#include "ManagedWriter.h"


class Multiplexer {
private:
    Multiplexer();

public:
    static Multiplexer& instance() {
        return multiplexer;
    }

    ManagedReader& getManagedReader() {
        return managedReader;
    }

    ManagedWriter& getManagedWriter() {
        return managedWriter;
    }

    ManagedExceptions& getManagedExceptions() {
        return managedExceptions;
    }

    ManagedTimer& getManagedTimer() {
        return managedTimer;
    }

    static void init(int argc, char* argv[]);
    static void start();
    static void stop();

private:
    static void stoponsig(int sig);

    inline void tick();

    static Multiplexer multiplexer;

    ManagedReader managedReader;
    ManagedWriter managedWriter;
    ManagedExceptions managedExceptions;
    ManagedTimer managedTimer;

    static bool running;
    static bool stopped;
};


#endif // MULTIPLEXER_H
