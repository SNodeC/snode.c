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

#ifndef CORE_EVENTRECEIVER_H
#define CORE_EVENTRECEIVER_H

namespace core {
    class DescriptorEventDispatcher;
} // namespace core

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h" // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class Observer {
    public:
        bool isObserved() {
            return observationCounter > 0;
        }

        void observed() {
            observationCounter++;
        }

        void unObserved() {
            observationCounter--;
        }

        int getObservationCounter() {
            return observationCounter;
        }

        virtual void unobservedEvent() = 0;

    private:
        int observationCounter = 0;
    };

    class EventReceiver : virtual public Observer {
        EventReceiver(const EventReceiver&) = delete;
        EventReceiver& operator=(const EventReceiver&) = delete;

    public:
        class TIMEOUT {
        public:
            static const utils::Timeval DEFAULT;
            static const utils::Timeval DISABLE;
            static const utils::Timeval MAX;
        };

    protected:
        explicit EventReceiver(DescriptorEventDispatcher& descriptorEventDispatcher, const utils::Timeval& timeout = TIMEOUT::DISABLE);
        virtual ~EventReceiver() = default;

    public:
        int getRegisteredFd();
        virtual bool continueImmediately() const = 0;

        void enable(int fd);
        void disable();
        bool isEnabled() const;

        void suspend();
        void resume();
        bool isSuspended() const;

        void triggered(const utils::Timeval& currentTime);
        void trigger(const utils::Timeval& currentTime);

        void disabled();

        virtual void terminate();

        void setTimeout(const utils::Timeval& timeout);
        utils::Timeval getTimeout(const utils::Timeval& currentTime) const;
        void checkTimeout(const utils::Timeval& currentTime);

    private:
        virtual void dispatchEvent() = 0;
        virtual void timeoutEvent() = 0;

        DescriptorEventDispatcher& descriptorEventDispatcher;

        int registeredFd = -1;

        bool enabled = false;
        bool suspended = false;

        utils::Timeval lastTriggered;
        utils::Timeval maxInactivity;
        const utils::Timeval initialTimeout;
    };

} // namespace core

#endif // CORE_EVENTRECEIVER_H
