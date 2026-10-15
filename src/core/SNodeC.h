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

#ifndef CORE_SNODEC_H
#define CORE_SNODEC_H

#include "core/State.h" // IWYU pragma: export
#include "core/TickStatus.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Timeval.h"

#include <climits>
#include <cstddef>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace core {

    /**
     * One event loop per calling thread; existing single-threaded calls retain their signatures.
     *
     * Call init in each thread before constructing its receivers, sockets or timers.
     * The first init supplies the process arguments (which must outlive the runtime).
     * The first start or tick bootstraps process configuration once, after setup is complete.
     * Configuration construction, destruction and mutation must be serialized by the application;
     * reconfigure is supported only with one initialized loop. Daemonize before spawning workers.
     *
     * start, tick, state, stop and free act on the calling thread's loop. Resources and
     * deferred callbacks stay on their construction thread; this API does not transfer them.
     * Process signals request shutdown in all loops. Descriptor waits are capped at 100 ms
     * so a signal delivered to another thread is observed without a separate signal thread.
     *
     * start drains and frees its loop before returning. Manual tick users call free after
     * stopping dispatch. Pair every thread's init with start or free before thread exit.
     * The last initialized loop releases process services, including configuration and logs;
     * keep the initial thread initialized until workers join if their configuration outlives dispatch.
     * free is idempotent. A stopped loop/runtime cannot be restarted.
     * Register linked protocol factories and middleware in each owning thread.
     */
    class SNodeC {
    public:
        SNodeC() = delete;
        ~SNodeC() = delete;

        void* operator new(std::size_t count) = delete;

        static void init(int argc, char* argv[]);
        static int start(const utils::Timeval& timeOut = {LONG_MAX, 0});
        // Calling loop RUNNING and the only initialized loop. Configuration is shared;
        // serialize its construction/mutation. Failed parsing is not rolled back.
        [[nodiscard]] static bool reconfigure();
        static void stop();
        static TickStatus tick(const utils::Timeval& timeOut = 0);
        static void free();

        static State state();
    };

} // namespace core

#endif // CORE_SNODEC_H
