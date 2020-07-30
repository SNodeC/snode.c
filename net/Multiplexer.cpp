#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cassert>
#include <cerrno> // for EINTR, errno
#include <csignal>
#include <sys/time.h> // for timeval

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Logger.h"
#include "Multiplexer.h"

Multiplexer Multiplexer::multiplexer;
bool Multiplexer::running = false;
bool Multiplexer::stopped = true;
bool Multiplexer::initialized = false;

Multiplexer::Multiplexer()
    : managedReader(readfds)
    , managedServer(readfds)
    , managedWriter(writefds)
    , managedExceptions(exceptfds) {
}

void Multiplexer::tick() {
    managedReader.observeStartedDescriptors();
    managedWriter.observeStartedDescriptors();
    managedExceptions.observeStartedDescriptors();
    managedServer.observeStartedDescriptors();

    int maxFd = managedReader.getMaxFd();
    maxFd = std::max(managedServer.getMaxFd(), maxFd);
    maxFd = std::max(managedWriter.getMaxFd(), maxFd);
    maxFd = std::max(managedExceptions.getMaxFd(), maxFd);

    fd_set wExceptfds = exceptfds;
    fd_set wWritefds = writefds;
    fd_set wReadfds = readfds;

    struct timeval tv = managedTimer.getNextTimeout();

    if (maxFd >= 0 || !managedTimer.empty()) {
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
    } else {
        Multiplexer::stopped = true;
    }

    managedReader.unobserveStopedDescriptors();
    managedWriter.unobserveStopedDescriptors();
    managedExceptions.unobserveStopedDescriptors();
    managedServer.unobserveStopedDescriptors();
}

void Multiplexer::init(int argc, char* argv[]) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGQUIT, Multiplexer::stoponsig);
    signal(SIGHUP, Multiplexer::stoponsig);
    signal(SIGINT, Multiplexer::stoponsig);
    signal(SIGTERM, Multiplexer::stoponsig);

    Logger::init(argc, argv);

    Multiplexer::initialized = true;
}

void Multiplexer::start() {
    if (!initialized) {
        std::cerr << "ERROR: snode.c not initialized. Use Multiplexer::init(argc, argv) before Multiplexer::start()." << std::endl;
        exit(1);
    }

    Multiplexer::stopped = false;

    if (!Multiplexer::running) {
        Multiplexer::running = true;

        while (!Multiplexer::stopped) {
            Multiplexer::multiplexer.tick();
        };

        Multiplexer::running = false;

        Multiplexer::instance().getManagedReader().observeStartedDescriptors();
        Multiplexer::instance().getManagedWriter().observeStartedDescriptors();
        Multiplexer::instance().getManagedExceptions().observeStartedDescriptors();
        Multiplexer::instance().getManagedServer().observeStartedDescriptors();

        Multiplexer::instance().getManagedReader().unobserveStopedDescriptors();
        Multiplexer::instance().getManagedWriter().unobserveStopedDescriptors();
        Multiplexer::instance().getManagedExceptions().unobserveStopedDescriptors();
        Multiplexer::instance().getManagedServer().unobserveStopedDescriptors();

        Multiplexer::instance().getManagedReader().removeManagedDescriptors();
        Multiplexer::instance().getManagedWriter().removeManagedDescriptors();
        Multiplexer::instance().getManagedExceptions().removeManagedDescriptors();
        Multiplexer::instance().getManagedServer().removeManagedDescriptors();

        Multiplexer::instance().getManagedTimer().getNextTimeout();
    }
}

void Multiplexer::stop() {
    Multiplexer::stopped = true;
}

void Multiplexer::stoponsig([[maybe_unused]] int sig) {
    stop();
}
