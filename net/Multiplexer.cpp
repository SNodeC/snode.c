#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Multiplexer.h"


Multiplexer Multiplexer::multiplexer;
bool Multiplexer::running = false;

void Multiplexer::tick() {
    fd_set exceptfds = exceptionManager.getFdSet();
    fd_set writefds = writeManager.getFdSet();
    fd_set readfds = readManager.getFdSet();
    
    int maxFd = readManager.getMaxFd();
    maxFd = writeManager.getMaxFd() > maxFd ? writeManager.getMaxFd() : maxFd;
    maxFd = exceptionManager.getMaxFd() > maxFd ? writeManager.getMaxFd() : maxFd;
    
    struct timeval tv = timerManager.getNextTimeout();
    
    int retval = select(maxFd + 1, &readfds, &writefds, &exceptfds, &tv);
    
    if (retval < 0 && errno != EINTR) {
        perror("Select");
        stop();
    } else {
        timerManager.dispatch();
        if (retval > 0) {
            retval = readManager.dispatch(readfds, retval);
        }
        if (retval > 0) {
            retval = writeManager.dispatch(writefds, retval);
        }
        if (retval > 0) {
            retval = exceptionManager.dispatch(exceptfds, retval);
        }
    }
}


void Multiplexer::run()
{
    if (!Multiplexer::running) {
        Multiplexer::running = true;
        
        while (Multiplexer::running){
            Multiplexer::instance().tick();
        };
        
        multiplexer.readManager.clear();
        multiplexer.writeManager.clear();
        multiplexer.exceptionManager.clear();
    }
}


void Multiplexer::stop()
{
    Multiplexer::running = false;
}
