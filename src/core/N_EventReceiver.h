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

namespace core {}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

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
        enum class State { NEW, ACTIVE, INACTIVE, STOPPED };

        class Timeout {
        public:
            static const utils::Timeval DEFAULT;
            static const utils::Timeval DISABLE;
        };

        N_EventReceiver(EventDispatcher& eventDispatcher, const utils::Timeval& timeout = Timeout::DISABLE);

        virtual ~N_EventReceiver() = default;

        void setTimeout(const utils::Timeval& timeout);
        utils::Timeval getTimeout(const utils::Timeval& currentTime) const;

        void triggered();

        void activate();
        void inactivate();
        void stop();

        virtual void terminate();

        bool isNew() const;
        bool isActive() const;
        bool isInactive() const;
        bool isStopped() const;

        EventDispatcher& eventDispatcher;

    private:
        virtual void dispatchEvent() = 0;
        virtual void timeoutEvent() = 0;

        virtual utils::Timeval continueIn() = 0;

        State state = State::NEW;

        utils::Timeval maxInactivity;
        utils::Timeval initialTimeout;
        utils::Timeval lastTriggered;
    };

} // namespace core

#endif // CORE_NEVENTRECEIVER_H
