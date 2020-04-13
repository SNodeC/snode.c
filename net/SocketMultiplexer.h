#ifndef SOCKETMULTIPLEXER_H
#define SOCKETMULTIPLEXER_H

#include "SocketReadManager.h"
#include "SocketWriteManager.h"
#include "SocketExceptionManager.h"
#include "TimerManager.h"


class SocketMultiplexer
{
private:
    
    SocketMultiplexer() {
    }
    
public:
    static SocketMultiplexer socketMultiplexer;
    
    static SocketMultiplexer& instance() {
        return socketMultiplexer;
    }
    
    void tick();
    
    SocketReadManager& getReadManager() {
        return readManager;
    }
    
    SocketWriteManager& getWriteManager() {
        return writeManager;
    }
    
    SocketExceptionManager& getExceptionManager() {
        return exceptionManager;
    }
    
protected:
    SocketReadManager readManager;
    SocketWriteManager writeManager;
    SocketExceptionManager exceptionManager;
    TimerManager timerManager;
};

#endif // SOCKETMULTIPLEXER_H
