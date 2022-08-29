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

#ifndef CORE_DESCRIPTOREVENTRECEIVER_H
#define CORE_DESCRIPTOREVENTRECEIVER_H

#include "core/EventReceiver.h" // IWYU pragma: export

namespace core {
    class DescriptorEventPublisher;
} // namespace core

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h" // IWYU pragma: export

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class Observer {
    public:
        virtual ~Observer() = default;

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
        : virtual protected Observer
        , public EventReceiver {
    public:
        DescriptorEventReceiver(const DescriptorEventReceiver&) = delete;

        DescriptorEventReceiver& operator=(const DescriptorEventReceiver&) = delete;

        class TIMEOUT {
        public:
            static const utils::Timeval DEFAULT;
            static const utils::Timeval DISABLE;
            static const utils::Timeval MAX;
        };

        enum DISP_TYPE { RD = 0, WR = 1, EX = 2 };

        DescriptorEventReceiver(const std::string& name,
                                core::DescriptorEventReceiver::DISP_TYPE dispType,
                                const utils::Timeval& timeout = TIMEOUT::DISABLE);

        int getRegisteredFd();

        void enable(int fd);
        void disable();

        void suspend();
        void resume();

        bool isEnabled() const;
        bool isSuspended() const;

        void setTimeout(const utils::Timeval& timeout);
        utils::Timeval getTimeout(const utils::Timeval& currentTime) const;

        void checkTimeout(const utils::Timeval& currentTime);

        virtual void terminate();

    private:
        void onEvent(const utils::Timeval& currentTime) final;
        void triggered(const utils::Timeval& currentTime);
        void setEnabled(const utils::Timeval& currentTime);
        void setDisabled();

        virtual void dispatchEvent() = 0;
        virtual void timeoutEvent() = 0;

        DescriptorEventPublisher& descriptorEventPublisher;

        int observedFd = -1;

        bool enabled = false;
        bool suspended = false;

        utils::Timeval lastTriggered;
        utils::Timeval maxInactivity;
        const utils::Timeval initialTimeout;

        int eventCounter = 0;

        friend class DescriptorEventPublisher;
    };

} // namespace core

#endif // CORE_DESCRIPTOREVENTRECEIVER_H
