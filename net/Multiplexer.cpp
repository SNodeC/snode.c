#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cassert>
#include <csignal>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "Multiplexer.h"


Multiplexer Multiplexer::multiplexer;
bool Multiplexer::running = false;
bool Multiplexer::stopped = false;


Multiplexer::Multiplexer() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, Multiplexer::stoponsig);
    signal(SIGTERM, Multiplexer::stoponsig);
}


void Multiplexer::tick() {
    fd_set exceptfds = managedExceptions.getFdSet();
    fd_set writefds = managedWriter.getFdSet();
    fd_set readfds = managedReader.getFdSet();

    int maxFd = managedReader.getMaxFd();
    maxFd = managedWriter.getMaxFd() > maxFd ? managedWriter.getMaxFd() : maxFd;
    maxFd = managedExceptions.getMaxFd() > maxFd ? managedWriter.getMaxFd() : maxFd;

    struct timeval tv = managedTimer.getNextTimeout();

    int retval = select(maxFd + 1, &readfds, &writefds, &exceptfds, &tv);

    if (retval >= 0) {
        managedTimer.dispatch();
        if (retval > 0) {
            retval = managedReader.dispatch(readfds, retval);
        }
        if (retval > 0) {
            retval = managedWriter.dispatch(writefds, retval);
        }
        if (retval > 0) {
            retval = managedExceptions.dispatch(exceptfds, retval);
        }
        assert(retval == 0);
    } else if (errno != EINTR) {
        PLOG(ERROR) << "select";
        stop();
    }
}


void Multiplexer::init(int argc, char* argv[]) {
    Logger::init(argc, argv);
}


void Multiplexer::start() {
    Multiplexer::stopped = false;

    if (!Multiplexer::running) {
        Multiplexer::running = true;

        while (!Multiplexer::stopped) {
            Multiplexer::multiplexer.tick();
        };

        Multiplexer::running = false;
    }
}


void Multiplexer::stop() {
    Multiplexer::stopped = true;
}


void Multiplexer::stoponsig(int sig) {
    stop();
}
