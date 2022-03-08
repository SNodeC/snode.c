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

#include "DescriptorEventPublisher.h"

#include "core/DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <type_traits> // for add_const<>::type

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::select {

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

    DescriptorEventPublisher::DescriptorEventPublisher(FdSet& fdSet)
        : fdSet(fdSet) {
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

    void DescriptorEventPublisher::muxOff(int fd) {
        fdSet.clr(fd);
    }

    void DescriptorEventPublisher::publishActiveEvents() {
        for (const auto& [fd, eventReceivers] : observedEventReceivers) {
            core::DescriptorEventReceiver* eventReceiver = eventReceivers.front();
            if (fdSet.isSet(fd) && !eventReceiver->isSuspended()) {
                eventCounter++;
                eventReceiver->publish();
            }
        }
    }

} // namespace core::select
