#ifndef TIMERMANAGER_H
#define TIMERMANAGER_H

<<<<<<< HEAD
#include <list>

#include <Timer.h>

class TimerManager {
public:
    TimerManager();
    struct timeval getNextTimeout();
    void process();
    
    void add(Timer* timer);
    void remove(Timer* timer);
    
protected:
    std::list<Timer*> timerList;
    std::list<Timer*> addedList;
    std::list<Timer*> removedList;
    
private:
    bool timerListDirty;
    
    class lttimernodecomparator {
    public:
        bool operator() (const Timer* t1, const Timer* t2) const;
    };
=======
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Timer.h"


class TimerManager
{
public:
	TimerManager ();
	
	struct timeval getNextTimeout ();
	
	void dispatch ();
	
	void remove (Timer *timer);
	
	void add (Timer *timer);

private:
	std::list<Timer *> timerList;
	std::list<Timer *> addedList;
	std::list<Timer *> removedList;
	
	class lttimernode
	{
	public:
		bool operator() (const Timer *t1, const Timer *t2) const;
	};
	
	
	class timernode_equality
	{
	public:
		timernode_equality (Timer *timer) : timer(timer)
		{}
		
		bool operator() (const Timer *timer) const;
	
	private:
		Timer *timer;
	};
	
	bool timerListDirty;
>>>>>>> MatthiasKoettritsch
};

#endif // TIMERMANAGER_H
