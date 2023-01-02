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

#ifndef CORE_TIMEREVENTRECEIVER_H
#define CORE_TIMEREVENTRECEIVER_H

#include "core/EventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h" // IWYU pragma: export

namespace core {
    class Timer;
    class TimerEventPublisher;
} // namespace core

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class TimerEventReceiver : public EventReceiver {
    public:
        TimerEventReceiver(const TimerEventReceiver&) = delete;

        TimerEventReceiver& operator=(const TimerEventReceiver&) = delete;

        void restart();

    protected:
        TimerEventReceiver(const std::string& name, const utils::Timeval& delay);
        ~TimerEventReceiver() override;

        utils::Timeval getTimeout() const;

        void enable();
        void update();
        void cancel();

    private:
        void onEvent(const utils::Timeval& currentTime) final;

        virtual void dispatchEvent() = 0;
        virtual void unobservedEvent() = 0;

        void setTimer(Timer* timer);

        void updateTimeout();

        TimerEventPublisher& timerEventPublisher;

        Timer* timer = nullptr;
        utils::Timeval absoluteTimeout;
        utils::Timeval delay;

        friend class Timer;
        friend class TimerEventPublisher;
    };

} // namespace core

#endif // CORE_TIMEREVENTRECEIVER_H
