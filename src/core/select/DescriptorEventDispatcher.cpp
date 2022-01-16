/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "core/select/DescriptorEventDispatcher.h"

#include "core/EventReceiver.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <iterator>    // for reverse_iterator
#include <type_traits> // for add_const<>::type
#include <utility>     // for tuple_element<>::type, pair

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::select {

    DescriptorEventDispatcher::FdSet::FdSet() {
        zero();
    }

    void DescriptorEventDispatcher::FdSet::set(int fd) {
        FD_SET(fd, &registered);
    }

    void DescriptorEventDispatcher::FdSet::clr(int fd) {
        FD_CLR(fd, &registered);
        FD_CLR(fd, &active);
    }

    int DescriptorEventDispatcher::FdSet::isSet(int fd) const {
        return FD_ISSET(fd, &active);
    }

    void DescriptorEventDispatcher::FdSet::zero() {
        FD_ZERO(&registered);
        FD_ZERO(&active);
    }

    fd_set& DescriptorEventDispatcher::FdSet::get() {
        active = registered;
        return active;
    }

    void DescriptorEventDispatcher::modAdd(EventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();
        fdSet.set(fd);
    }

    void DescriptorEventDispatcher::modDel(EventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();
        fdSet.clr(fd);
    }

    void DescriptorEventDispatcher::modOn(EventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();
        fdSet.set(fd);
    }

    void DescriptorEventDispatcher::modOff(EventReceiver* eventReceiver) {
        int fd = eventReceiver->getRegisteredFd();
        fdSet.clr(fd);
    }

    int DescriptorEventDispatcher::getInterestCount() const {
        int maxFd = -1;

        if (!observedEventReceiver.empty()) {
            maxFd = observedEventReceiver.rbegin()->first;
        }

        return maxFd;
    }

    fd_set& DescriptorEventDispatcher::getFdSet() {
        return fdSet.get();
    }

    void DescriptorEventDispatcher::dispatchActiveEvents(const utils::Timeval& currentTime) {
        for (const auto& [fd, eventReceivers] : observedEventReceiver) {
            core::EventReceiver* eventReceiver = eventReceivers.front();
            if (fdSet.isSet(fd) && !eventReceiver->continueImmediately() && !eventReceiver->isSuspended()) {
                eventCounter++;
                eventReceiver->dispatch(currentTime);
            }
        }
    }

    void DescriptorEventDispatcher::unobserveDisabledEvents(const utils::Timeval& currentTime) {
        bool doCompress = false;

        for (const auto& [fd, eventReceivers] : disabledEventReceiver) {
            for (core::EventReceiver* eventReceiver : eventReceivers) {
                observedEventReceiver[fd].remove(eventReceiver);
                if (observedEventReceiver[fd].empty()) {
                    modDel(eventReceiver);
                    observedEventReceiver.erase(fd);
                } else if (!observedEventReceiver[fd].front()->isSuspended()) {
                    modOn(observedEventReceiver[fd].front());
                    observedEventReceiver[fd].front()->triggered(currentTime);
                } else {
                    modOff(observedEventReceiver[fd].front());
                }
                eventReceiver->disabled();
                if (eventReceiver->getObservationCounter() == 0) {
                    eventReceiver->unobservedEvent();
                    doCompress = true;
                }
            }
        }

        disabledEventReceiver.clear();

        if (doCompress) {
        }
    }

} // namespace core::select
