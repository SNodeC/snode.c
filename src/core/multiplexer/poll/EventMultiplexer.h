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

#ifndef CORE_POLL_EVENTDISPATCHER_H
#define CORE_POLL_EVENTDISPATCHER_H

#include "core/EventMultiplexer.h" // IWYU pragma: export

namespace core {
    class DescriptorEventReceiver;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/system/poll.h" // IWYU pragma: export

#include <cstddef>
#include <functional>
#include <limits>
#include <new>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core::multiplexer::poll {

    class PollFdsManager {
    private:
        template <typename T, typename S>
        class VarSizeTypeAllocator {
        public:
            // Standard allocator typedefs
            using value_type = T;
            using pointer = T*;
            using const_pointer = const T*;
            using void_pointer = void*;
            using const_void_pointer = const void*;
            // Use nfds_t as the size_type.
            using size_type = S;
            using difference_type = std::ptrdiff_t;

            // Rebind allocator to type U.
            template <typename U>
            struct rebind {
                using other = VarSizeTypeAllocator<U, S>;
            };

            // Constructors
            VarSizeTypeAllocator() noexcept {
            }

            VarSizeTypeAllocator(const VarSizeTypeAllocator&) noexcept = default;

            template <typename U>
            VarSizeTypeAllocator(const VarSizeTypeAllocator<U, S>& /*unused*/) noexcept {
            }

            ~VarSizeTypeAllocator() {
            }

            // Allocate memory for n objects of type T.
            pointer allocate(size_type n) {
                if (n > max_size()) {
                    throw std::bad_alloc();
                }
                // Allocate using the global operator new.
                return static_cast<pointer>(::operator new(n * sizeof(T)));
            }

            // Deallocate memory for n objects of type T.
            void deallocate(pointer p, size_type /*n*/) noexcept {
                ::operator delete(p);
            }

            // Returns the maximum number of T objects that can be allocated.
            size_type max_size() const noexcept {
                return std::numeric_limits<size_type>::max() / sizeof(T);
            }

            // Construct an object of type U at the given memory location.
            template <typename U, typename... Args>
            void construct(U* p, Args&&... args) {
                ::new (static_cast<void*>(p)) U(std::forward<Args>(args)...);
            }

            // Destroy an object of type U.
            template <typename U>
            void destroy(U* p) noexcept {
                p->~U();
            }

            // Equality comparisons.
            template <typename U>
            bool operator==(const VarSizeTypeAllocator<U, S>& /*unused*/) const noexcept {
                return true;
            }

            template <typename U>
            bool operator!=(const VarSizeTypeAllocator<U, S>& /*unused*/) const noexcept {
                return false;
            }
        };

    public:
        using pollfd_vector = std::vector<pollfd, VarSizeTypeAllocator<pollfd, nfds_t>>;

        struct PollFdIndex {
            pollfd_vector::size_type index;
            short events;
        };

        using pollfdindex_map = std::unordered_map<int,
                                                   PollFdIndex,
                                                   std::hash<int>,
                                                   std::equal_to<>,
                                                   VarSizeTypeAllocator<std::pair<const int, PollFdIndex>, nfds_t>>;

        explicit PollFdsManager();

        void muxAdd(core::DescriptorEventReceiver* eventReceiver, short event);
        void muxDel(int fd, short event);
        void muxOn(const core::DescriptorEventReceiver* eventReceiver, short event);
        void muxOff(const DescriptorEventReceiver* eventReceiver, short event);

        pollfd* getEvents();

        const pollfdindex_map& getPollFdIndices() const;

        nfds_t getCurrentSize() const;

    private:
        nfds_t nextIndex = 0;
        void compress();

        pollfd_vector pollfds;
        pollfdindex_map pollFdIndices;
    };

    class EventMultiplexer : public core::EventMultiplexer {
    public:
        EventMultiplexer();
        ~EventMultiplexer() override = default;

    private:
        int monitorDescriptors(utils::Timeval& tickTimeOut, const sigset_t& sigMask) override;
        void spanActiveEvents(int activeDescriptorCount) override;

        PollFdsManager pollFdsManager;
    };

} // namespace core::multiplexer::poll

#endif // CORE_POLL_EVENTDISPATCHER_H
