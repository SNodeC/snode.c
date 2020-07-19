#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cassert>
#include <csignal>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "Multiplexer.h"


Multiplexer Multiplexer::multiplexer;
bool Multiplexer::running = false;
bool Multiplexer::stopped = true;


Multiplexer::Multiplexer()
    : managedReader(readfds)
    , managedServer(readfds)
    , managedWriter(writefds)
    , managedExceptions(exceptfds) {
}


void Multiplexer::tick() {
    int maxFd = managedReader.getMaxFd();
    maxFd = std::max(managedServer.getMaxFd(), maxFd);
    maxFd = std::max(managedWriter.getMaxFd(), maxFd);
    maxFd = std::max(managedExceptions.getMaxFd(), maxFd);

    fd_set wExceptfds = exceptfds;
    fd_set wWritefds = writefds;
    fd_set wReadfds = readfds;

    struct timeval tv = managedTimer.getNextTimeout();

    int retval = select(maxFd + 1, &wReadfds, &wWritefds, &wExceptfds, &tv);

    if (retval >= 0) {
        managedTimer.dispatch();
        if (retval > 0) {
            retval = managedReader.dispatch(wReadfds, retval);
        }
        if (retval > 0) {
            retval = managedServer.dispatch(wReadfds, retval);
        }
        if (retval > 0) {
            retval = managedWriter.dispatch(wWritefds, retval);
        }
        if (retval > 0) {
            retval = managedExceptions.dispatch(wExceptfds, retval);
        }
        assert(retval == 0);
    } else if (errno != EINTR) {
        PLOG(ERROR) << "select";
        stop();
    }
}


void Multiplexer::init(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGQUIT, Multiplexer::stoponsig);
    signal(SIGHUP, Multiplexer::stoponsig);
    signal(SIGINT, Multiplexer::stoponsig);
    signal(SIGTERM, Multiplexer::stoponsig);

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

        Multiplexer::instance().getManagedReader().addDescriptors();
        Multiplexer::instance().getManagedWriter().addDescriptors();
        Multiplexer::instance().getManagedExceptions().addDescriptors();
        Multiplexer::instance().getManagedServer().addDescriptors();

        Multiplexer::instance().getManagedReader().removeDescriptors();
        Multiplexer::instance().getManagedWriter().removeDescriptors();
        Multiplexer::instance().getManagedExceptions().removeDescriptors();
        Multiplexer::instance().getManagedServer().removeDescriptors();

        Multiplexer::instance().getManagedReader().removeManagedDescriptors();
        Multiplexer::instance().getManagedWriter().removeManagedDescriptors();
        Multiplexer::instance().getManagedExceptions().removeManagedDescriptors();
        Multiplexer::instance().getManagedServer().removeManagedDescriptors();
    }
}


void Multiplexer::stop() {
    Multiplexer::stopped = true;
}


void Multiplexer::stoponsig([[maybe_unused]] int sig) {
    stop();
}
