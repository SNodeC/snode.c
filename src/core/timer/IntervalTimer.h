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

#ifndef NET_TIMER_INTERVALTIMER_H
#define NET_TIMER_INTERVALTIMER_H

#include "core/TimerEventReceiver.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::timer {

    class IntervalTimer : public core::TimerEventReceiver {
        IntervalTimer(const IntervalTimer&) = delete;

        IntervalTimer& operator=(const IntervalTimer& timer) = delete;

    public:
        IntervalTimer(const std::function<void(const void*, const std::function<void()>& stop)>& dispatcher,
                      const utils::Timeval& timeout,
                      const void* arg);

        IntervalTimer(const std::function<void(const void*)>& dispatcher, const utils::Timeval& timeout, const void* arg);

        ~IntervalTimer() override = default;

        void dispatch(const utils::Timeval& currentTime) override;

    private:
        void unobservedEvent() override;

        std::function<void(const void*, const std::function<void()>&)> dispatcherS = nullptr;
        std::function<void(const void*)> dispatcherC = nullptr;

        const void* arg;
    };

} // namespace core::timer

#endif // NET_TIMER_INTERVALTIMER_H
