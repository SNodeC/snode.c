/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#ifndef NET_EVENTRECEIVER_H
#define NET_EVENTRECEIVER_H

#include "utils/Timeval.h" // for operator-, operator<, operator>=

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <climits>
#include <ctime>
#include <sys/time.h> // for timeval

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventDispatcher;

    class Observer {
    public:
        bool isObserved() {
            return observationCounter > 0;
        }

    private:
        virtual void unobservedEvent() = 0;

    protected:
        void observed() {
            observationCounter++;
        }

        void unObserved() {
            observationCounter--;
        }

        int observationCounter = 0;

        friend class EventDispatcher;
    };

    class EventReceiver : virtual public Observer {
        EventReceiver(const EventReceiver&) = delete;
        EventReceiver& operator=(const EventReceiver&) = delete;

    protected:
        class TIMEOUT {
        public:
            static const long DEFAULT = -1;
            static const long DISABLE = LONG_MAX;
        };

        explicit EventReceiver(EventDispatcher& descriptorEventDispatcher, long timeout = TIMEOUT::DISABLE);

        virtual ~EventReceiver() = default;

    protected:
        void enable(int fd);
        void disable();
        bool isEnabled() const;

        void suspend();
        void resume();
        bool isSuspended() const;

        virtual void terminate();

        void setTimeout(long timeout);

        void triggered(struct timeval lastTriggered = {time(nullptr), 0});

    private:
        void disabled();

        struct timeval getTimeout() const;
        struct timeval getLastTriggered();

        virtual void dispatchEvent() = 0;
        virtual void timeoutEvent() = 0;

        virtual bool continueImmediately() = 0;

        EventDispatcher& descriptorEventDispatcher;

        int fd = -1;

        bool _enabled = false;
        bool _suspended = false;

        struct timeval lastTriggered = {0, 0};

        long maxInactivity = LONG_MAX;
        const long initialTimeout;

        friend class EventDispatcher;

        // New API
        enum class State { NEW, ACTIVE, INACTIVE, STOPED } state = State::NEW;

        class Timeout {
        public:
            static const struct timeval constexpr DEFAULT = {-1, 0};
            static const struct timeval constexpr DISABLE = {LONG_MAX, 0};
        };

        bool isNew() {
            return state == State::NEW;
        }

        void n_activate() {
            state = State::ACTIVE;
        }

        bool isActive() {
            return state == State::ACTIVE;
        }

        void n_inactivate() {
            state = State::INACTIVE;
        }

        bool isInactive() {
            return state == State::INACTIVE;
        }

        void n_stop() {
            state = State::STOPED;
        }

        bool isStopped() {
            return state == State::STOPED;
        }

        struct timeval n_maxInactivity = {LONG_MAX, 0};
        struct timeval n_initialTimeout = n_maxInactivity;
        struct timeval n_lastTriggered = {0, 0};

        void n_setTimeout(struct timeval timeout) {
            if (timeout == Timeout::DEFAULT) {
                n_maxInactivity = n_initialTimeout;
            } else {
                n_maxInactivity = timeout;
            }
        }

        struct timeval n_getTimeout() {
            struct timeval currentTime = {0, 0};
            gettimeofday(&currentTime, NULL);

            return std::max(n_maxInactivity - (currentTime - n_lastTriggered), {0, 0});
        }
    };

} // namespace core

#endif // NET_EVENTRECEIVER_H
