#ifndef SOCKETMULTIPLEXER_H
#define SOCKETMULTIPLEXER_H

#include "SocketReadManager.h"
#include "SocketWriteManager.h"
#include "SocketExceptionManager.h"
#include "TimerManager.h"

#include <iostream>


class SocketMultiplexer
{
private:
    SocketMultiplexer() {
//        atexit(SocketMultiplexer::run);
    }
    
public:
    static SocketMultiplexer& instance() {
        return socketMultiplexer;
    }
    
    SocketReadManager& getReadManager() {
        return readManager;
    }
    
    SocketWriteManager& getWriteManager() {
        return writeManager;
    }
    
    SocketExceptionManager& getExceptionManager() {
        return exceptionManager;
    }
    
    TimerManager& getTimerManager() {
        return timerManager;
    }

    static void run();
    
private:
    void tick();
    
    static SocketMultiplexer socketMultiplexer;
    
    SocketReadManager readManager;
    SocketWriteManager writeManager;
    SocketExceptionManager exceptionManager;
    TimerManager timerManager;
};


#endif // SOCKETMULTIPLEXER_H
