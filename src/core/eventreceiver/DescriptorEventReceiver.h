/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef CORE_EVENTRECEIVER_DESCRIPTOREVENTRECEIVER_H
#define CORE_EVENTRECEIVER_DESCRIPTOREVENTRECEIVER_H

namespace core {
    class DescriptorEventPublisher;
} // namespace core

#include "core/EventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h" // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::eventreceiver {

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

    class DescriptorEventReceiver
        : virtual public Observer
        , public EventReceiver {
        DescriptorEventReceiver(const DescriptorEventReceiver&) = delete;
        DescriptorEventReceiver& operator=(const DescriptorEventReceiver&) = delete;

    public:
        class TIMEOUT {
        public:
            static const utils::Timeval DEFAULT;
            static const utils::Timeval DISABLE;
            static const utils::Timeval MAX;
        };

    protected:
        explicit DescriptorEventReceiver(DescriptorEventPublisher& descriptorEventPublisher,
                                         const utils::Timeval& timeout = TIMEOUT::DISABLE);
        ~DescriptorEventReceiver() = default;

    public:
        int getRegisteredFd();

        void enable(int fd);
        void disable();
        bool isEnabled() const;

        void suspend();
        void resume();
        bool isSuspended() const;

        void dispatch(const utils::Timeval& currentTime) override;

        void triggered(const utils::Timeval& currentTime);

        void disabled();

        virtual void terminate();

        void setTimeout(const utils::Timeval& timeout);
        utils::Timeval getTimeout(const utils::Timeval& currentTime) const;
        void checkTimeout(const utils::Timeval& currentTime);

    private:
        virtual void dispatchEvent() = 0;
        virtual void timeoutEvent() = 0;

        DescriptorEventPublisher& descriptorEventPublisher;

        int registeredFd = -1;

        bool enabled = false;
        bool suspended = false;

    public:
        utils::Timeval publisedTime;
        utils::Timeval lastTriggered;
        utils::Timeval maxInactivity;
        const utils::Timeval initialTimeout;

        int eventCounter = 0;
    };

} // namespace core::eventreceiver

#endif // CORE_EVENTRECEIVER_DESCRIPTOREVENTRECEIVER_H
