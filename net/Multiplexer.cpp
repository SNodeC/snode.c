#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Multiplexer.h"


Multiplexer Multiplexer::multiplexer;
bool Multiplexer::running = false;
bool Multiplexer::stopped = false;

void Multiplexer::tick() {
    fd_set exceptfds = managedExceptions.getFdSet();
    fd_set writefds = managedWriter.getFdSet();
    fd_set readfds = managedReader.getFdSet();

    int maxFd = managedReader.getMaxFd();
    maxFd = managedWriter.getMaxFd() > maxFd ? managedWriter.getMaxFd() : maxFd;
    maxFd = managedExceptions.getMaxFd() > maxFd ? managedWriter.getMaxFd() : maxFd;

    struct timeval tv = managedTimer.getNextTimeout();

    int retval = select(maxFd + 1, &readfds, &writefds, &exceptfds, &tv);

    if (retval < 0 && errno != EINTR) {
        perror("Select");
        stop();
    } else {
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
        if (retval > 0) {
            std::cerr << "Select: not all descriptors processed - left: " << retval << std::endl;
        }
    }
}


void Multiplexer::start() {
    if (!Multiplexer::running) {
        Multiplexer::running = true;

        while (!Multiplexer::stopped) {
            Multiplexer::instance().tick();
        };
        Multiplexer::instance().tick();

        Multiplexer::running = false;
    }
}


void Multiplexer::stop() {
    Multiplexer::stopped = true;
}
