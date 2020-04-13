#include "TimerManager.h"

#include <sys/time.h>

struct timeval TimerManager::getNextTimeout() {
    struct timeval tv;
    
    tv.tv_sec = 20L;
    tv.tv_usec = 0L;
    
    return tv;
}

int TimerManager::process() {
    return 0;
}
