#ifndef SOCKETMULTIPLEXER_H
#define SOCKETMULTIPLEXER_H

#include "ReadManager.h"
#include "WriteManager.h"
#include "ExceptionManager.h"
#include "TimerManager.h"


class Multiplexer
{
private:
    Multiplexer() {}
    
    ~Multiplexer() {}
    
    
public:
    static Multiplexer& instance() {
        return socketMultiplexer;
    }
    
    ReadManager& getReadManager() {
        return readManager;
    }
    
    WriteManager& getWriteManager() {
        return writeManager;
    }
    
    ExceptionManager& getExceptionManager() {
        return exceptionManager;
    }
    
    TimerManager& getTimerManager() {
        return timerManager;
    }

    static void run();
    
    static void stop();
    
private:
    void tick();
    
    static Multiplexer socketMultiplexer;
    
    ReadManager readManager;
    WriteManager writeManager;
    ExceptionManager exceptionManager;
    TimerManager timerManager;
    
    static bool running;
};


#endif // SOCKETMULTIPLEXER_H
