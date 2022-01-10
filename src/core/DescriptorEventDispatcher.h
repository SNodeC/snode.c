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

#ifndef CORE_DESCRIPTOREVENTDISPATCHER_H
#define CORE_DESCRIPTOREVENTDISPATCHER_H

namespace core {
    class EventReceiver;
    class EventDispatcher;
} // namespace core

namespace utils {
    class Timeval;
} // namespace utils

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class DescriptorEventDispatcher {
        DescriptorEventDispatcher(const DescriptorEventDispatcher&) = delete;
        DescriptorEventDispatcher& operator=(const DescriptorEventDispatcher&) = delete;

    protected:
        DescriptorEventDispatcher() = default;

    public:
        virtual void enable(EventReceiver* eventReceiver) = 0;
        virtual void disable(EventReceiver* eventReceiver) = 0;
        virtual void suspend(EventReceiver* eventReceiver) = 0;
        virtual void resume(EventReceiver* eventReceiver) = 0;

    private:
        virtual unsigned long getEventCounter() const = 0;

        virtual void stop() = 0;
    };

} // namespace core

#endif // CORE_DESCRIPTOREVENTDISPATCHER_H
