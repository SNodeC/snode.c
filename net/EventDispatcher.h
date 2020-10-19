/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EVENTDISPATCHER_H
#define EVENTDISPATCHER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <climits>
#include <ctime>
#include <list>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Descriptor.h"
#include "EventReceiver.h"
#include "FdSet.h"
#include "timer/Timer.h"

namespace net {

    class EventLoop;

    template <typename EventReceiverT>
    static bool contains(std::list<EventReceiverT*>& eventReceivers, EventReceiverT*& eventReceiver) {
        typename std::list<EventReceiverT*>::iterator it = std::find(eventReceivers.begin(), eventReceivers.end(), eventReceiver);

        return it != eventReceivers.end();
    }

    template <typename EventReceiverT>
    class EventDispatcher {
    public:
        using EventReceiver = EventReceiverT;

        explicit EventDispatcher(FdSet& fdSet, long maxInactivity) // NOLINT(google-runtime-references)
            : fdSet(fdSet)
            , maxInactivity(maxInactivity) {
        }

        EventDispatcher(const EventDispatcher&) = delete;

        EventDispatcher& operator=(const EventDispatcher&) = delete;

        void enable(EventReceiver* eventReceiver) {
            if (contains(disabledEventReceiver, eventReceiver)) {
                // same tick
                disabledEventReceiver.remove(eventReceiver);
            } else if (!eventReceiver->isEnabled() && !contains(enabledEventReceiver, eventReceiver)) {
                // normal
                enabledEventReceiver.push_back(eventReceiver);
                eventReceiver->enabled();
            }
        }

        void disable(EventReceiver* eventReceiver) {
            if (contains(enabledEventReceiver, eventReceiver)) {
                // same tick
                enabledEventReceiver.remove(eventReceiver);
                eventReceiver->disabled();
            } else if (eventReceiver->isEnabled() && !contains(disabledEventReceiver, eventReceiver)) {
                // normal
                disabledEventReceiver.push_back(eventReceiver);
            }
        }

        long getTimeout() const {
            return maxInactivity;
        }

        unsigned long getEventCounter() {
            return eventCounter;
        }

    private:
        int getMaxFd() {
            int maxFd = -1;

            if (!observedEventReceiver.empty()) {
                maxFd = observedEventReceiver.rbegin()->first;
            }

            return maxFd;
        }

        struct timeval observeEnabledEvents() {
            struct timeval nextTimeout = {LONG_MAX, 0};

            for (EventReceiver* eventReceiver : enabledEventReceiver) {
                int fd = dynamic_cast<Descriptor*>(eventReceiver)->getFd();
                observedEventReceiver[fd].push_front(eventReceiver);
                fdSet.set(fd);
                nextTimeout = std::min(nextTimeout, eventReceiver->getTimeout());
            }
            enabledEventReceiver.clear();

            fdSet.prepare();

            return nextTimeout;
        }

        struct timeval dispatchActiveEvents(struct timeval currentTime) {
            struct timeval nextInactivityTimeout {
                LONG_MAX, 0
            };

            for (const auto& [fd, eventReceivers] : observedEventReceiver) {
                EventReceiver* eventReceiver = eventReceivers.front();
                struct timeval maxInactivity = eventReceiver->getTimeout();
                if (fdSet.isSet(fd)) {
                    eventCounter++;
                    dispatchEventTo(eventReceiver);
                    eventReceiver->setLastTriggered(currentTime);
                    nextInactivityTimeout = std::min(nextInactivityTimeout, maxInactivity);
                } else {
                    struct timeval inactivity = currentTime - eventReceiver->getLastTriggered();
                    if (inactivity >= maxInactivity) {
                        eventReceiver->disable();
                    } else {
                        nextInactivityTimeout = std::min(maxInactivity - inactivity, nextInactivityTimeout);
                    }
                }
            }

            return nextInactivityTimeout;
        }

        void unobserveDisabledEvents() {
            for (EventReceiver* eventReceiver : disabledEventReceiver) {
                int fd = dynamic_cast<Descriptor*>(eventReceiver)->getFd();
                observedEventReceiver[fd].remove(eventReceiver);
                if (observedEventReceiver[fd].empty()) {
                    observedEventReceiver.erase(fd);
                    fdSet.clr(fd);
                } else {
                    observedEventReceiver[fd].front()->setLastTriggered({time(nullptr), 0});
                }
                eventReceiver->disabled();
            }
            disabledEventReceiver.clear();
        }

        void disableObservedEvents() {
            for (auto& [fd, eventReceivers] : observedEventReceiver) {
                for (EventReceiver* eventReceiver : eventReceivers) {
                    disabledEventReceiver.push_back(eventReceiver);
                }
            }
        }

        virtual void dispatchEventTo(EventReceiver*) = 0;

        std::map<int, std::list<EventReceiver*>> observedEventReceiver;

        std::list<EventReceiver*> enabledEventReceiver;
        std::list<EventReceiver*> disabledEventReceiver;

        FdSet& fdSet;

        long maxInactivity;

        unsigned long eventCounter = 0;

        friend class EventLoop;
    };

} // namespace net

#endif // EVENTDISPATCHER_H
