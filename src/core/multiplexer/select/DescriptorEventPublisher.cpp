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

#include "core/multiplexer/select/DescriptorEventPublisher.h"

#include "core/DescriptorEventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

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
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif
        FD_ZERO(&registered);
        FD_ZERO(&active);
#ifdef __clang__
#pragma clang diagnostic pop
#endif
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

    int DescriptorEventPublisher::spanActiveEvents() {
        int count = 0;

        for (auto& [fd, eventReceivers] : observedEventReceivers) {
            if (fdSet.isSet(fd)) {
                LOG(TRACE) << "SELECT " << getName() << " DEP fired";
                core::DescriptorEventReceiver* eventReceiver = eventReceivers.front();
                eventCounter++;
                eventReceiver->span();
                count++;
            }
        }

        return count;
    }

} // namespace core::select
