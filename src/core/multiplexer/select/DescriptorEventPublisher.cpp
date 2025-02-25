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

#include "core/multiplexer/select/DescriptorEventPublisher.h"

#include "core/DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::multiplexer::select {

    FdSet::FdSet() {
        zero();
    }

    void FdSet::set(int fd) {
        FD_SET(fd, &registered);
    }

    void FdSet::clr(int fd) {
        FD_CLR(fd, &registered);
        FD_CLR(fd, &active);
    }

    int FdSet::isSet(int fd) const {
        return FD_ISSET(fd, &active);
    }

    void FdSet::zero() {
        FD_ZERO(&registered);
        FD_ZERO(&active);
    }

    fd_set& FdSet::get() {
        active = registered;
        return active;
    }

    DescriptorEventPublisher::DescriptorEventPublisher(const std::string& name, FdSet& fdSet)
        : core::DescriptorEventPublisher(name)
        , fdSet(fdSet) {
    }

    void DescriptorEventPublisher::muxAdd(core::DescriptorEventReceiver* eventReceiver) {
        fdSet.set(eventReceiver->getRegisteredFd());
    }

    void DescriptorEventPublisher::muxDel(int fd) {
        fdSet.clr(fd);
    }

    void DescriptorEventPublisher::muxOn(core::DescriptorEventReceiver* eventReceiver) {
        fdSet.set(eventReceiver->getRegisteredFd());
    }

    void DescriptorEventPublisher::muxOff(core::DescriptorEventReceiver* eventReceiver) {
        fdSet.clr(eventReceiver->getRegisteredFd());
    }

    void DescriptorEventPublisher::spanActiveEvents() {
        for (auto& [fd, eventReceivers] : observedEventReceiverLists) {
            if (fdSet.isSet(fd) != 0) {
                core::DescriptorEventReceiver* eventReceiver = eventReceivers.front();
                eventCounter++;
                eventReceiver->span();
            }
        }
    }

} // namespace core::multiplexer::select
