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

#include "SingleshotTimer.h"

#include "core/TimerEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::timer {

    SingleshotTimer::SingleshotTimer(const std::function<void(const void*)>& dispatcher,
                                     const utils::Timeval& timeout,
                                     const void* arg,
                                     const std::string& name)
        : TimerEventReceiver(name, timeout)
        , dispatcher(dispatcher)
        , arg(arg) {
    }

    void SingleshotTimer::dispatch(const utils::Timeval& currentTime) {
        LOG(INFO) << "Timer: Dispatch delta = " << (currentTime - getTimeout()).msd() << " ms";
        dispatcher(arg);
        cancel();
    }

    void SingleshotTimer::unobservedEvent() {
        dispatcher = nullptr;
        delete this;
    }

} // namespace core::timer
