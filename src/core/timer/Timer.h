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

#ifndef NET_TIMER_TIMER_H
#define NET_TIMER_TIMER_H

#include "core/Timer.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h" // IWYU pragma: export

namespace core {
    class TimerEventReceiver;
} // namespace core

#include <functional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::timer {

    class Timer : public core::Timer {
    public:
        using core::Timer::Timer;

        ~Timer() override;

        Timer& operator=(const Timer&) = delete;

        Timer& operator=(Timer&& timer) noexcept = default;

        static Timer intervalTimer(const std::function<void(const std::function<void()>&)>& dispatcher, const utils::Timeval& timeout);

        static Timer intervalTimer(const std::function<void()>& dispatcher, const utils::Timeval& timeout);

        static Timer singleshotTimer(const std::function<void()>& dispatcher, const utils::Timeval& timeout);
    };

} // namespace core::timer

#endif // NET_TIMER_TIMER_H
