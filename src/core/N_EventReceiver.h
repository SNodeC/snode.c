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

#ifndef CORE_NEVENTRECEIVER_H
#define CORE_NEVENTRECEIVER_H

#include "utils/Timeval.h" // for operator-, operator<, operator>=

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <climits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class EventDispatcher;

    class N_Observer {
    public:
        virtual ~N_Observer() = default;

    protected:
        bool isObserved() {
            return observationCounter > 0;
        }

        void observed() {
            observationCounter++;
        }

        void unObserved() {
            observationCounter--;
        }

        void destroy() {
            delete this;
        }

    private:
        virtual void unobservedEvent() = 0;

        int observationCounter = 0;
    };

    class N_EventReceiver : virtual public N_Observer {
        N_EventReceiver(const N_EventReceiver&) = delete;
        N_EventReceiver& operator=(const N_EventReceiver&) = delete;

    protected:
        class Timeout {
        public:
            static const ttime::Timeval DEFAULT;
            static const ttime::Timeval DISABLE;
        };

        enum class State { NEW, ACTIVE, INACTIVE, STOPPED } state = State::NEW;

        N_EventReceiver(EventDispatcher& descriptorEventDispatcher, const ttime::Timeval& timeout = Timeout::DISABLE);

        virtual ~N_EventReceiver() = default;
        void setTimeout(const ttime::Timeval& timeout);
        ttime::Timeval getTimeout() const;

        void triggered();

        void activate();
        void inactivate();
        void stop();

        virtual void terminate();

        bool isNew() const;
        bool isActive() const;
        bool isInactive() const;
        bool isStopped() const;

    private:
        virtual void dispatchEvent() = 0;
        virtual void timeoutEvent() = 0;

        virtual ttime::Timeval continueIn() = 0;

        EventDispatcher& descriptorEventDispatcher;

        ttime::Timeval maxInactivity{{LONG_MAX, 0}};
        ttime::Timeval initialTimeout = maxInactivity;
        ttime::Timeval lastTriggered = {{0, 0}};
    };

} // namespace core

#endif // CORE_NEVENTRECEIVER_H
