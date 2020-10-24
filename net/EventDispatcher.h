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
    class EventDispatcher {
    public:
        using EventReceiver = EventReceiverT;

        struct EventReceiverFdPair {
            EventReceiverFdPair(EventReceiver* eventReceiver, int fd)
                : eventReceiver(eventReceiver)
                , fd(fd) {
            }

            bool operator==(const EventReceiver* eventReceiver) const {
                return this->eventReceiver == eventReceiver;
            }

            EventReceiverT* eventReceiver;
            int fd;
        };

        static bool contains(std::list<EventReceiverFdPair>& eventReceivers, EventReceiverT*& eventReceiver) {
            return std::find(eventReceivers.begin(), eventReceivers.end(), eventReceiver) != eventReceivers.end();
        }

        explicit EventDispatcher(FdSet& fdSet, long maxInactivity) // NOLINT(google-runtime-references)
            : fdSet(fdSet)
            , maxInactivity(maxInactivity) {
        }

        EventDispatcher(const EventDispatcher&) = delete;

        EventDispatcher& operator=(const EventDispatcher&) = delete;

        void enable(EventReceiver* eventReceiver, int fd) {
            if (contains(disabledEventReceiver, eventReceiver)) {
                // same tick
                disabledEventReceiver.remove_if([eventReceiver](const EventReceiverFdPair& eventReceiverEntry) {
                    return eventReceiverEntry.eventReceiver == eventReceiver;
                });
            } else if (!eventReceiver->isEnabled() && !contains(enabledEventReceiver, eventReceiver)) {
                // normal
                enabledEventReceiver.push_back(EventReceiverFdPair(eventReceiver, fd));
                eventReceiver->enabled();
            }
        }

        void disable(EventReceiver* eventReceiver, int fd) {
            if (contains(enabledEventReceiver, eventReceiver)) {
                // same tick
                enabledEventReceiver.remove_if([eventReceiver](const EventReceiverFdPair& eventReceiverEntry) {
                    return eventReceiverEntry.eventReceiver == eventReceiver;
                });
                eventReceiver->disabled();
            } else if (eventReceiver->isEnabled() && !contains(disabledEventReceiver, eventReceiver)) {
                // normal
                disabledEventReceiver.push_back(EventReceiverFdPair(eventReceiver, fd));
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

            for (EventReceiverFdPair& eventReceiverEntry : enabledEventReceiver) {
                EventReceiver* eventReceiver = eventReceiverEntry.eventReceiver;
                int fd = eventReceiverEntry.fd;

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
            for (EventReceiverFdPair& eventReceiverEntry : disabledEventReceiver) {
                EventReceiver* eventReceiver = eventReceiverEntry.eventReceiver;
                int fd = eventReceiverEntry.fd;

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
                    disabledEventReceiver.push_back(EventReceiverFdPair(eventReceiver, fd));
                }
            }
        }

        std::map<int, std::list<EventReceiver*>> observedEventReceiver;

        std::list<EventReceiverFdPair> enabledEventReceiver;
        std::list<EventReceiverFdPair> disabledEventReceiver;

        std::list<EventReceiver*> unobservedEventReceiver;

        FdSet& fdSet;

        long maxInactivity;

        unsigned long eventCounter = 0;

        friend class EventLoop;
    }; // namespace net

} // namespace net

#endif // EVENTDISPATCHER_H
