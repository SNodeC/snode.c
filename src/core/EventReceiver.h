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

#include "utils/Timeval.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class DescriptorEventDispatcher;

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

        friend class EventReceiver;
        friend class DescriptorEventDispatcher;
    };

    class EventReceiver : virtual public Observer {
        EventReceiver(const EventReceiver&) = delete;
        EventReceiver& operator=(const EventReceiver&) = delete;

    protected:
        class TIMEOUT {
        public:
            static const utils::Timeval DEFAULT;
            static const utils::Timeval DISABLE;
        };

        explicit EventReceiver(DescriptorEventDispatcher& descriptorEventDispatcher, const utils::Timeval& timeout = TIMEOUT::DISABLE);

        virtual ~EventReceiver() = default;

    protected:
        void enable(int fd);
        void disable();
        bool isEnabled() const;

        void suspend();
        void resume();
        bool isSuspended() const;

        void setTimeout(const utils::Timeval& timeout);

        virtual void terminate();

    private:
        void triggered(const utils::Timeval& currentTime);
        void trigger(const utils::Timeval& currentTime);

        void checkTimeout(const utils::Timeval& currentTime);

        void disabled();

        utils::Timeval getTimeout(const utils::Timeval& currentTime) const;

        virtual void dispatchEvent() = 0;
        virtual void timeoutEvent() = 0;

        virtual bool continueImmediately() = 0;

        DescriptorEventDispatcher& eventDispatcher;

        int fd = -1;

        bool _enabled = false;
        bool _suspended = false;

        utils::Timeval lastTriggered;
        utils::Timeval maxInactivity;
        const utils::Timeval initialTimeout;

        friend class DescriptorEventDispatcher;
    };

} // namespace core

#endif // CORE_EVENTRECEIVER_H
