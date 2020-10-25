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

    private:
        class EventReceiverList : public std::list<EventReceiver*> {
        public:
            using std::list<EventReceiver*>::front;
            using std::list<EventReceiver*>::begin;
            using std::list<EventReceiver*>::end;

            bool contains(EventReceiver* eventReceiver) {
                return std::find(std::list<EventReceiver*>::begin(), end(), eventReceiver) != end();
            }
        };

    public:
        explicit EventDispatcher(FdSet& fdSet, long maxInactivity) // NOLINT(google-runtime-references)
            : fdSet(fdSet)
            , maxInactivity(maxInactivity) {
        }

        EventDispatcher(const EventDispatcher&) = delete;

        EventDispatcher& operator=(const EventDispatcher&) = delete;

        void enable(EventReceiver* eventReceiver, int fd) {
            if (disabledEventReceiver[fd].contains(eventReceiver)) {
                // same tick
                disabledEventReceiver[fd].remove(eventReceiver);
            } else if (!eventReceiver->isEnabled() &&
                       (!enabledEventReceiver.contains(fd) || !enabledEventReceiver[fd].contains(eventReceiver))) {
                // normal
                enabledEventReceiver[fd].push_back(eventReceiver);
                eventReceiver->enabled(fd);
            } else {
                LOG(WARNING) << "EventReceiver double enable";
            }
        }

        void disable(EventReceiver* eventReceiver, int fd) {
            if (enabledEventReceiver[fd].contains(eventReceiver)) {
                // same tick
                enabledEventReceiver[fd].remove(eventReceiver);
                eventReceiver->disabled();
            } else if (eventReceiver->isEnabled() &&
                       (!disabledEventReceiver.contains(fd) || !disabledEventReceiver[fd].contains(eventReceiver))) {
                // normal
                disabledEventReceiver[fd].push_back(eventReceiver);
            } else {
                LOG(WARNING) << "EventReceiver double disable";
            }
        }

        void suspend(EventReceiver* eventReceiver, int fd) {
            if (!eventReceiver->isSuspended()) {
                eventReceiver->suspended();
                if (observedEventReceiver.contains(fd) && observedEventReceiver[fd].front() == eventReceiver) {
                    fdSet.clr(fd, true);
                }
            } else {
                LOG(WARNING) << "EventReceiver double suspend";
            }
        }

        void resume(EventReceiver* eventReceiver, int fd) {
            if (eventReceiver->isSuspended()) {
                eventReceiver->resumed();
                if (observedEventReceiver.contains(fd) && observedEventReceiver[fd].front() == eventReceiver) {
                    fdSet.set(fd);
                }
            } else {
                LOG(WARNING) << "EventReceiver double resume";
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

            for (auto [fd, eventReceivers] : enabledEventReceiver) {
                for (EventReceiver* eventReceiver : eventReceivers) {
                    observedEventReceiver[fd].push_front(eventReceiver);
                    if (!eventReceiver->isSuspended()) {
                        fdSet.set(fd);
                        nextTimeout = std::min(nextTimeout, eventReceiver->getTimeout());
                    } else {
                        fdSet.clr(fd, true);
                    }
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
                    eventReceiver->triggered(currentTime);
                    nextInactivityTimeout = std::min(nextInactivityTimeout, maxInactivity);
                } else {
                    struct timeval inactivity = currentTime - eventReceiver->getLastTriggered();
                    if (inactivity >= maxInactivity) {
                        eventReceiver->timeoutEvent();
                        eventReceiver->disable();
                    } else {
                        nextInactivityTimeout = std::min(maxInactivity - inactivity, nextInactivityTimeout);
                    }
                }
            }

            return nextInactivityTimeout;
        }

        void unobserveDisabledEvents() {
            for (auto [fd, eventReceivers] : disabledEventReceiver) {
                for (EventReceiver* eventReceiver : eventReceivers) {
                    observedEventReceiver[fd].remove(eventReceiver);
                    if (observedEventReceiver[fd].empty() || observedEventReceiver[fd].front()->isSuspended()) {
                        if (observedEventReceiver[fd].empty()) {
                            observedEventReceiver.erase(fd);
                        }
                        fdSet.clr(fd, true);
                    } else {
                        fdSet.set(fd);
                        observedEventReceiver[fd].front()->triggered();
                    }
                    eventReceiver->disabled();
                    if (eventReceiver->observationCounter == 0) {
                        unobservedEventReceiver.push_back(eventReceiver);
                    }
                    eventReceiver->fd = -1;
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
                    disabledEventReceiver[fd].push_back(eventReceiver);
                }
            }
        }

        std::map<int, EventReceiverList> enabledEventReceiver;
        std::map<int, EventReceiverList> observedEventReceiver;
        std::map<int, EventReceiverList> disabledEventReceiver;
        std::list<EventReceiver*> unobservedEventReceiver;

        FdSet& fdSet;

        long maxInactivity;

        unsigned long eventCounter = 0;

        friend class EventLoop;
    }; // namespace net

} // namespace net

#endif // EVENTDISPATCHER_H
