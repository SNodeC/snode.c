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

#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"
#include "core/TimerEventPublisher.h"
#include "core/TimerEventReceiver.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::timer {

    class IntervalTimer : public core::TimerEventReceiver {
        IntervalTimer& operator=(const IntervalTimer& timer) = delete;

    public:
        IntervalTimer(const std::function<void(const void*, const std::function<void()>& stop)>& dispatcher,
                      const utils::Timeval& timeout,
                      const void* arg)
            : core::TimerEventReceiver(timeout)
            , dispatcherS(dispatcher)
            , arg(arg) {
        }

        IntervalTimer(const std::function<void(const void*)>& dispatcher, const utils::Timeval& timeout, const void* arg)
            : core::TimerEventReceiver(timeout)
            , dispatcherC(dispatcher)
            , arg(arg) {
        }

        ~IntervalTimer() override = default;

        void dispatch(const utils::Timeval& currentTime) override {
            if (dispatcherS) {
                LOG(INFO) << "Timer: Dispatch delta = " << (currentTime - getTimeout()).msd() << " ms";
                bool stop = false;
                dispatcherS(arg, [&stop]() -> void {
                    stop = true;
                });
                if (stop) {
                    cancel();
                } else {
                    update();
                }
            } else if (dispatcherC) {
                LOG(INFO) << "Timer: Dispatch delta = " << (currentTime - getTimeout()).msd() << " ms";
                dispatcherC(arg);
                update();
            }
        }

    private:
        void update() {
            EventLoop::instance().getEventMultiplexer().getTimerEventPublisher().update(this);
        }

        void unobservedEvent() override {
            dispatcherS = nullptr;
            dispatcherC = nullptr;
            delete this;
        }

        std::function<void(const void*, const std::function<void()>&)> dispatcherS = nullptr;
        std::function<void(const void*)> dispatcherC = nullptr;

        const void* arg;
    };

} // namespace core::timer

#endif // NET_TIMER_INTERVALTIMER_H
