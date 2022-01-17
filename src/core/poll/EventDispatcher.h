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

#ifndef CORE_POLL_EVENTDISPATCHER_H
#define CORE_POLL_EVENTDISPATCHER_H

#include "core/EventDispatcher.h"
#include "core/poll/DescriptorEventDispatcher.h"
#include "core/poll/TimerEventDispatcher.h"

namespace core {
    class EventReceiver;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint> // for uint32_t
#include <poll.h>
#include <unordered_map>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::poll {

    struct PollFdIndex {
        using pollfds_size_type = std::vector<pollfd>::size_type;

        pollfds_size_type index; // cppcheck-suppress unusedStructMember
        uint8_t refCount;        // cppcheck-suppress unusedStructMember
    };

    class PollFds {
    public:
        using pollfds_size_type = std::vector<pollfd>::size_type;
        explicit PollFds();

        void add(std::unordered_map<int, EventReceiver*, std::hash<int>>& pollEvents, core::EventReceiver* eventReceiver, short event);
        void del(std::unordered_map<int, EventReceiver*, std::hash<int>>& pollEvents, core::EventReceiver* eventReceiver, short event);

        void modOn(std::unordered_map<int, EventReceiver*, std::hash<int>>& pollEvents, core::EventReceiver* eventReceiver, short event);
        void modOff(std::unordered_map<int, EventReceiver*, std::hash<int>>& pollEvents, core::EventReceiver* eventReceiver, short event);

        void compress();

        pollfd* getEvents();

        std::unordered_map<int, PollFdIndex>& getPollFdIndices();

        nfds_t getInterestCount() const;

    private:
        std::vector<pollfd> pollfds;
        std::unordered_map<int, PollFdIndex> pollFdIndices;

        uint32_t interestCount;
    };

    class EventDispatcher : public core::EventDispatcher {
        EventDispatcher(const EventDispatcher&) = delete;
        EventDispatcher& operator=(const EventDispatcher&) = delete;

    public:
        EventDispatcher();
        ~EventDispatcher() = default;

        core::DescriptorEventDispatcher& getDescriptorEventDispatcher(core::EventDispatcher::DISP_TYPE dispType) override;
        core::TimerEventDispatcher& getTimerEventDispatcher() override;

        TickStatus dispatch(const utils::Timeval& tickTimeOut, bool stopped) override;
        void stop() override;

    private:
        int getInterestCount();
        utils::Timeval getNextTimeout(const utils::Timeval& currentTime);

        void observeEnabledEvents();
        void unobserveDisabledEvents(const utils::Timeval& currentTime);

        void dispatchActiveEvents(int count, const utils::Timeval& currentTime);

    public:
        core::poll::DescriptorEventDispatcher eventDispatcher[3];

    private:
        core::poll::TimerEventDispatcher timerEventDispatcher;

        PollFds pollFds;
    };

} // namespace core::poll

#endif // CORE_POLL_EVENTDISPATCHER_H

#ifdef NOBLOCK

/* Event types that can be polled for.  These bits may be set in `events'
   to indicate the interesting event types; they will appear in `revents'
   to indicate the status of the file descriptor.  */
#define POLLIN 0x001  /* There is data to read.  */
#define POLLPRI 0x002 /* There is urgent data to read.  */
#define POLLOUT 0x004 /* Writing now will not block.  */

#if defined __USE_XOPEN || defined __USE_XOPEN2K8
/* These values are defined in XPG4.2.  */
#define POLLRDNORM 0x040 /* Normal data may be read.  */
#define POLLRDBAND 0x080 /* Priority data may be read.  */
#define POLLWRNORM 0x100 /* Writing now will not block.  */
#define POLLWRBAND 0x200 /* Priority data may be written.  */
#endif

#ifdef __USE_GNU
/* These are extensions for Linux.  */
#define POLLMSG 0x400
#define POLLREMOVE 0x1000
#define POLLRDHUP 0x2000
#endif

/* Event types always implicitly polled for.  These bits need not be set in
   `events', but they will appear in `revents' to indicate the status of
   the file descriptor.  */
#define POLLERR 0x008  /* Error condition.  */
#define POLLHUP 0x010  /* Hung up.  */
#define POLLNVAL 0x020 /* Invalid polling request.  */

#endif
