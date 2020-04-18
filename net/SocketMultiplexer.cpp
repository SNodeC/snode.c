#include <stdio.h>
#include "SocketMultiplexer.h"


SocketMultiplexer SocketMultiplexer::socketMultiplexer;

void SocketMultiplexer::tick() {
    fd_set exceptfds = exceptionManager.getFdSet();
    fd_set writefds = writeManager.getFdSet();
    fd_set readfds = readManager.getFdSet();
    
    int maxFd = readManager.getMaxFd();
    maxFd = writeManager.getMaxFd() > maxFd ? writeManager.getMaxFd() : maxFd;
    maxFd = exceptionManager.getMaxFd() > maxFd ? writeManager.getMaxFd() : maxFd;
    
    struct timeval tv = timerManager.getNextTimeout();
    
    int retval = select(maxFd + 1, &readfds, &writefds, &exceptfds, &tv);
    
    if (retval < 0) {
        perror("Select:");
    } else {
        timerManager.process();
        if (retval > 0) {
            retval = readManager.process(readfds, retval);
        }
        if (retval > 0) {
            retval = writeManager.process(writefds, retval);
        }
        if (retval > 0) {
            retval = exceptionManager.process(exceptfds, retval);
        }
    }
}


void SocketMultiplexer::run()
{
    while(1) {
        this->tick();
    }
}
