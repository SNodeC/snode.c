/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef CORE_DESCRIPTOREVENTPUBLISHER_H
#define CORE_DESCRIPTOREVENTPUBLISHER_H

namespace core {
    class DescriptorEventReceiver;
} // namespace core

namespace utils {
    class Timeval;
} // namespace utils

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list> // IWYU pragma: export
#include <map>  // IWYU pragma: export
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class DescriptorEventPublisher {
    public:
    protected:
        DescriptorEventPublisher(std::string name);

    public:
        DescriptorEventPublisher() = delete;

        virtual ~DescriptorEventPublisher();

        DescriptorEventPublisher(const DescriptorEventPublisher&) = delete;
        DescriptorEventPublisher& operator=(const DescriptorEventPublisher&) = delete;

        void enable(DescriptorEventReceiver* descriptorEventReceiver);
        void disable(DescriptorEventReceiver* descriptorEventReceiver);
        void suspend(DescriptorEventReceiver* descriptorEventReceiver);
        void resume(DescriptorEventReceiver* descriptorEventReceiver);

        virtual int spanActiveEvents() = 0;
        void checkTimedOutEvents(const utils::Timeval& currentTime);
        void releaseDisabledEvents(const utils::Timeval& currentTime);

        int getObservedEventReceiverCount() const;
        int maxFd() const;

        utils::Timeval getNextTimeout(const utils::Timeval& currentTime) const;

        void sigExit(int sigNum);
        void stop();

        const std::string& getName() const;

    protected:
        virtual void muxAdd(DescriptorEventReceiver* descriptorEventReceiver) = 0;
        virtual void muxDel(int fd) = 0;
        virtual void muxOn(DescriptorEventReceiver* descriptorEventReceiver) = 0;
        virtual void muxOff(DescriptorEventReceiver* descriptorEventReceiver) = 0;

        unsigned long eventCounter = 0;

        std::map<int, std::list<DescriptorEventReceiver*>> observedEventReceivers;

    private:
        bool observedEventReceiversDirty = false;

        std::string name;
    };

} // namespace core

#endif // CORE_DESCRIPTOREVENTPUBLISHER_H
