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

#ifndef CORE_SELECT_EVENTDISPATCHER_H
#define CORE_SELECT_EVENTDISPATCHER_H

#include "core/EventDispatcher.h"
#include "core/select/DescriptorEventDispatcher.h"
#include "core/select/TimerEventDispatcher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::select {

    class EventDispatcher : public core::EventDispatcher {
        EventDispatcher(const EventDispatcher&) = delete;
        EventDispatcher& operator=(const EventDispatcher&) = delete;

    public:
        EventDispatcher() = default;
        ~EventDispatcher() = default;

        core::DescriptorEventDispatcher& getDescriptorEventDispatcher(core::EventDispatcher::DISP_TYPE dispType) override;
        core::TimerEventDispatcher& getTimerEventDispatcher() override;

        TickStatus dispatch(const utils::Timeval& tickTimeOut, bool stopped) override;
        void stop() override;

    private:
        int getMaxFd();
        utils::Timeval getNextTimeout(const utils::Timeval& currentTime);

        void observeEnabledEvents();
        void dispatchActiveEvents(const utils::Timeval& currentTime);
        void unobserveDisabledEvents(const utils::Timeval& currentTime);

        core::select::DescriptorEventDispatcher eventDispatcher[3];
        core::select::TimerEventDispatcher timerEventDispatcher;
    };

} // namespace core::select

#endif // CORE_SELECT_EVENTDISPATCHER_H
