/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "core/TimerEventReceiver.h"

#include "core/EventLoop.h"
#include "core/EventMultiplexer.h"
#include "core/Timer.h"
#include "core/TimerEventPublisher.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    TimerEventReceiver::TimerEventReceiver(const std::string& name, const utils::Timeval& delay)
        : EventReceiver(name)
        , timerEventPublisher(EventLoop::instance().getEventMultiplexer().getTimerEventPublisher())
        , absoluteTimeout(utils::Timeval::currentTime() + delay)
        , delay(delay) {
    }

    void TimerEventReceiver::restart() {
        timerEventPublisher.erase(this);
        absoluteTimeout = utils::Timeval::currentTime() + delay;
        timerEventPublisher.insert(this);
    }

    TimerEventReceiver::~TimerEventReceiver() {
        if (timer != nullptr) {
            timer->removeTimerEventReceiver();
        }
    }

    utils::Timeval TimerEventReceiver::getTimeoutAbsolut() const {
        return absoluteTimeout;
    }

    utils::Timeval TimerEventReceiver::getTimeoutRelative(const utils::Timeval& currentTime) const {
        return absoluteTimeout > currentTime ? absoluteTimeout - currentTime : 0;
    }

    void TimerEventReceiver::enable() {
        if (core::eventLoopState() != core::State::STOPPING) {
            timerEventPublisher.insert(this);
        } else {
            LOG(WARNING) << "TimerEventReceiver - Enable after signal: Not enabled";
            delete this;
        }
    }

    void TimerEventReceiver::update() {
        timerEventPublisher.erase(this);
        absoluteTimeout += delay;
        timerEventPublisher.insert(this);
    }

    void TimerEventReceiver::cancel() {
        timerEventPublisher.remove(this);
    }

    void TimerEventReceiver::onEvent(const utils::Timeval& currentTime) {
        LOG(TRACE) << "TimerEventReceiver: Dispatch delta = " << (currentTime - getTimeoutAbsolut()).getMsd() << " ms";

        dispatchEvent();
    }

    void TimerEventReceiver::setTimer(Timer* timer) {
        this->timer = timer;
    }

} // namespace core
