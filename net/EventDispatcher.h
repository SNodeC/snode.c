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
#include "Logger.h"
#include "Timeval.h"

namespace net {

    class EventLoop;

    template <typename EventReceiverT>
    static bool contains(std::list<EventReceiverT*>& eventReceivers, EventReceiverT*& eventReceiver) {
        return std::find(eventReceivers.begin(), eventReceivers.end(), eventReceiver) != eventReceivers.end();
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

        void enable(EventReceiver* eventReceiver, int fd) {
            if (disabledEventReceiver.contains(eventReceiver)) {
                // same tick
                disabledEventReceiver.erase(eventReceiver);
            } else if (!eventReceiver->isEnabled() && !enabledEventReceiver.contains(eventReceiver)) {
                // normal
                enabledEventReceiver.emplace(eventReceiver, fd);
                eventReceiver->enabled();
            }
        }

        void disable(EventReceiver* eventReceiver, int fd) {
            if (enabledEventReceiver.contains(eventReceiver)) {
                // same tick
                enabledEventReceiver.erase(eventReceiver);
                eventReceiver->disabled();
            } else if (eventReceiver->isEnabled() && !disabledEventReceiver.contains(eventReceiver)) {
                // normal
                disabledEventReceiver.emplace(eventReceiver, fd);
            }
        }

        void suspend(EventReceiver* eventReceiver, int fd) {
            eventReceiver->suspended();

            if (observedEventReceiver.contains(fd) && observedEventReceiver[fd].front() == eventReceiver) {
                fdSet.clr(fd, true);
            }
        }

        void resume(EventReceiver* eventReceiver, int fd) {
            eventReceiver->resumed();

            if (observedEventReceiver.contains(fd) && observedEventReceiver[fd].front() == eventReceiver) {
                fdSet.set(fd);
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

            for (auto [eventReceiver, fd] : enabledEventReceiver) {
                observedEventReceiver[fd].push_front(eventReceiver);
                if (!eventReceiver->isSuspended()) {
                    fdSet.set(fd);
                    nextTimeout = std::min(nextTimeout, eventReceiver->getTimeout());
                } else {
                    fdSet.clr(fd, true);
                }
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
                    eventReceiver->dispatchEvent();
                    eventReceiver->setLastTriggered(currentTime);
                    nextInactivityTimeout = std::min(nextInactivityTimeout, maxInactivity);
                } else {
                    struct timeval inactivity = currentTime - eventReceiver->getLastTriggered();
                    if (inactivity >= maxInactivity) {
                        eventReceiver->timeoutEvent();
                        eventReceiver->disable(fd);
                    } else {
                        nextInactivityTimeout = std::min(maxInactivity - inactivity, nextInactivityTimeout);
                    }
                }
            }

            return nextInactivityTimeout;
        }

        void unobserveDisabledEvents() {
            for (auto [eventReceiver, fd] : disabledEventReceiver) {
                observedEventReceiver[fd].remove(eventReceiver);
                if (observedEventReceiver[fd].empty() || observedEventReceiver[fd].front()->isSuspended()) {
                    if (observedEventReceiver[fd].empty()) {
                        observedEventReceiver.erase(fd);
                    }
                    fdSet.clr(fd, true);
                } else {
                    fdSet.set(fd);
                    observedEventReceiver[fd].front()->setLastTriggered({time(nullptr), 0});
                }
                eventReceiver->disabled();
                if (eventReceiver->observationCounter == 0) {
                    unobservedEventReceiver.push_back(eventReceiver);
                }
            }
            disabledEventReceiver.clear();
        }

        void unobserveDisabledEventReceiver() {
            for (EventReceiver* eventReceiver : unobservedEventReceiver) {
                eventReceiver->unobserved();
            }
            unobservedEventReceiver.clear();
        }

        void disableObservedEvents() {
            for (auto& [fd, eventReceivers] : observedEventReceiver) {
                for (EventReceiver* eventReceiver : eventReceivers) {
                    disabledEventReceiver.emplace(eventReceiver, fd);
                }
            }
        }

        std::map<int, std::list<EventReceiver*>> observedEventReceiver;

        std::multimap<EventReceiver*, int> enabledEventReceiver;
        std::multimap<EventReceiver*, int> disabledEventReceiver;

        std::list<EventReceiver*> unobservedEventReceiver;

        FdSet& fdSet;

        long maxInactivity;

        unsigned long eventCounter = 0;

        friend class EventLoop;
    }; // namespace net

} // namespace net

#endif // EVENTDISPATCHER_H
