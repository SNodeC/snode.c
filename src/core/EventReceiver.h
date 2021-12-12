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

#include "utils/Timeval.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <climits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventDispatcher;

    class Observer {
    private:
        bool isObserved() {
            return observationCounter > 0;
        }

        void observed() {
            observationCounter++;
        }

        void unObserved() {
            observationCounter--;
        }

        virtual void unobservedEvent() = 0;

        int observationCounter = 0;

        friend class EventDispatcher;
        friend class EventReceiver;
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

        void triggered();

    private:
        void disabled();

        utils::Timeval getTimeout() const;
        utils::Timeval getLastTriggered();

        virtual void dispatchEvent() = 0;
        virtual void timeoutEvent() = 0;

        virtual bool continueImmediately() = 0;

        EventDispatcher& descriptorEventDispatcher;

        int fd = -1;

        bool _enabled = false;
        bool _suspended = false;

        utils::Timeval lastTriggered;

        long maxInactivity = LONG_MAX;
        const long initialTimeout;

        friend class EventDispatcher;
    };

} // namespace core

#endif // NET_EVENTRECEIVER_H
