/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MANAGER_H
#define MANAGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <list>
#include <map>
#include <stack>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "EventReceiver.h"

class EventLoop;

template <typename EventReceiver>
class EventDispatcher {
public:
    explicit EventDispatcher(fd_set& fdSet) // NOLINT(google-runtime-references)
        : fdSet(fdSet) {
    }

    EventDispatcher(const EventDispatcher&) = delete;

    EventDispatcher& operator=(const EventDispatcher&) = delete;

private:
    static bool contains(std::list<EventReceiver*>& events, EventReceiver*& event) {
        typename std::list<EventReceiver*>::iterator it = std::find(events.begin(), events.end(), event);

        return it != events.end();
    }

public:
    void enable(EventReceiver* eventReceiver) {
        if (!eventReceiver->isEnabled() && !EventDispatcher<EventReceiver>::contains(enabledEventReceiver, eventReceiver)) {
            enabledEventReceiver.push_back(eventReceiver);
            eventReceiver->enabled();
        }
    }

    void disable(EventReceiver* eventReceiver) {
        if (eventReceiver->isEnabled()) {
            if (EventDispatcher<EventReceiver>::contains(enabledEventReceiver, eventReceiver)) { // stop() on same tick as start()
                enabledEventReceiver.remove(eventReceiver);
                eventReceiver->disabled();
            } else if (!EventDispatcher<EventReceiver>::contains(disabledEventReceiver, eventReceiver)) { // stop() asynchronously
                disabledEventReceiver.push_back(eventReceiver);
            }
        }
    }

private:
    int getLargestFd() {
        int fd = -1;

        if (!observedEvents.empty()) {
            fd = observedEvents.rbegin()->first;
        }

        return fd;
    }

    void observeEnabledEvents() {
        for (EventReceiver* eventReceiver : enabledEventReceiver) {
            int fd = dynamic_cast<Descriptor*>(eventReceiver)->getFd();
            //            bool inserted = false;
            observedEvents[fd].push_back(eventReceiver);
            FD_SET(fd, &fdSet);
            /*
                        std::tie(std::ignore, inserted) = observedEvents.insert({fd, eventReceiver});
                        if (inserted) {
                            FD_SET(fd, &fdSet);
                        } else {
                            eventReceiver->disabled();
                        }
            */
        }
        enabledEventReceiver.clear();
    }

    void unobserveDisabledEvents() {
        for (EventReceiver* eventReceiver : disabledEventReceiver) {
            int fd = dynamic_cast<Descriptor*>(eventReceiver)->getFd();
            observedEvents[fd].remove(eventReceiver);
            if (observedEvents[fd].empty()) {
                observedEvents.erase(fd);
                FD_CLR(fd, &fdSet);
            }
            eventReceiver->disabled();
            eventReceiver->destructIfUnobserved();
        }
        disabledEventReceiver.clear();
    }

    void unobserveObservedEvents() {
        for (auto& [fd, eventReceivers] : observedEvents) {
            for (EventReceiver* eventReceiver : eventReceivers) {
                disabledEventReceiver.push_back(eventReceiver);
            }
        }
        unobserveDisabledEvents();
    }

protected:
    virtual int dispatch(const fd_set& fdSet, int counter) = 0;

    std::map<int, std::list<EventReceiver*>> observedEvents;

private:
    std::list<EventReceiver*> enabledEventReceiver;
    std::list<EventReceiver*> disabledEventReceiver;

    fd_set& fdSet;

public:
    using EventType = EventReceiver;

    friend class EventLoop;
};

#endif // MANAGER_H
