#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

/**
 * @todo write docs
 */
class TimerManager
{
public:
    struct timeval getNextTimeout();
    int process();
};

#endif // TIMERMANAGER_H
