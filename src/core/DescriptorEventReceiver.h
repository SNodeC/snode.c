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

#ifndef CORE_DESCRIPTOREVENTRECEIVER_H
#define CORE_DESCRIPTOREVENTRECEIVER_H

#include "core/EventReceiver.h" // IWYU pragma: export

namespace core {
    class DescriptorEventPublisher;
} // namespace core

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    class Observer {
    public:
        Observer() = default;
        Observer(Observer&) = delete;
        Observer(Observer&&) = delete;

        virtual ~Observer();

    protected:
        void observed();
        void unObserved();

        virtual void unobservedEvent() = 0;

    private:
        int observationCounter = 0;
    };

    class DescriptorEventReceiver
        : virtual protected Observer
        , public EventReceiver {
    public:
        struct TIMEOUT {
            static const utils::Timeval DEFAULT;
            static const utils::Timeval DISABLE;
            static const utils::Timeval MAX;
        };

        DescriptorEventReceiver(const std::string& name,
                                DescriptorEventPublisher& descriptorEventPublisher,
                                const utils::Timeval& timeout = TIMEOUT::DISABLE);

        int getRegisteredFd() const;

    protected:
        bool enable(int fd);
        void disable();

        void suspend();
        void resume();

    public:
        bool isEnabled() const;
        bool isSuspended() const;

        void setTimeout(const utils::Timeval& timeout);
        utils::Timeval getTimeout(const utils::Timeval& currentTime) const;

        void checkTimeout(const utils::Timeval& currentTime);

    private:
        void onEvent(const utils::Timeval& currentTime) final;
        void onSignal(int signum);

        void triggered(const utils::Timeval& currentTime);
        void setEnabled(const utils::Timeval& currentTime);
        void setDisabled();

        virtual void dispatchEvent() = 0;
        virtual void timeoutEvent() = 0;
        virtual void signalEvent(int signum) = 0;

        DescriptorEventPublisher& descriptorEventPublisher;

        int observedFd = -1;

        bool enabled = false;
        bool suspended = false;

        utils::Timeval lastTriggered;
        utils::Timeval maxInactivity;
        const utils::Timeval initialTimeout;

        int eventCounter = 0;

        friend class DescriptorEventPublisher;
    };

} // namespace core

#endif // CORE_DESCRIPTOREVENTRECEIVER_H
