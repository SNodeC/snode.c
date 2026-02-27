/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include "core/TimerEventPublisher.h"

#include "core/TimerEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <climits>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    utils::Timeval TimerEventPublisher::getNextTimeout(const utils::Timeval& currentTime) {
        utils::Timeval nextTimeout({LONG_MAX, 0});

        if (!timerList.empty()) {
            nextTimeout = (*(timerList.begin()))->getTimeoutRelative(currentTime);
        }

        return nextTimeout;
    }

    void TimerEventPublisher::spanActiveEvents(const utils::Timeval& currentTime) {
        for (TimerEventReceiver* timerEventReceiver : timerList) {
            if (timerEventReceiver->getTimeoutAbsolut() <= currentTime) {
                timerEventReceiver->span();
            } else {
                break;
            }
        }
    }

    void TimerEventPublisher::unobserveDisableEvents() {
        for (TimerEventReceiver* timerEventReceiver : removedList) {
            timerList.erase(timerEventReceiver);
            timerEventReceiver->unobservedEvent();
        }
        removedList.clear();
    }

    void TimerEventPublisher::remove(TimerEventReceiver* timer) {
        if (std::find(timerList.begin(), timerList.end(), timer) != timerList.end() &&
            std::find(removedList.begin(), removedList.end(), timer) == removedList.end()) {
            removedList.push_back(timer);
        }
    }

    void TimerEventPublisher::erase(TimerEventReceiver* timer) {
        timerList.erase(timer);
    }

    void TimerEventPublisher::insert(TimerEventReceiver* timer) {
        timerList.insert(timer);
    }

    bool TimerEventPublisher::empty() const {
        return timerList.empty();
    }

    void TimerEventPublisher::stop() {
        for (TimerEventReceiver* timer : timerList) { // cppcheck-suppress constVariablePointer
            remove(timer);
        }

        unobserveDisableEvents();
    }

    bool TimerEventPublisher::timernode_lt::operator()(const TimerEventReceiver* t1, const TimerEventReceiver* t2) const {
        return t1->getTimeoutAbsolut() < t2->getTimeoutAbsolut();
    }

} // namespace core
