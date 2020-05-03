#include <string.h>

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

