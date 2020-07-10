#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cassert>
#include <csignal>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "Multiplexer.h"


Multiplexer Multiplexer::multiplexer;
bool Multiplexer::running = false;
bool Multiplexer::stopped = false;


Multiplexer::Multiplexer()
    : managedReader(m_readfds)
    , managedWriter(m_writefds)
    , managedExceptions(m_exceptionfds) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, Multiplexer::stoponsig);
    signal(SIGTERM, Multiplexer::stoponsig);
}


void Multiplexer::tick() {
    int maxFd = managedReader.getMaxFd();
    maxFd = managedWriter.getMaxFd() > maxFd ? managedWriter.getMaxFd() : maxFd;
    maxFd = managedExceptions.getMaxFd() > maxFd ? managedWriter.getMaxFd() : maxFd;

    fd_set exceptfds = m_exceptionfds;
    fd_set writefds = m_writefds;
    fd_set readfds = m_readfds;

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
