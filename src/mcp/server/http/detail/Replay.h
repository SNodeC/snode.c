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

#pragma once
#include "mcp/server/http/Router.h"
#include "utils/Timeval.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#endif
namespace mcp::server::http::detail {
    /// 2025 transport Resumability and Redelivery: globally unique, stream-bound ids.
    class Replay {
    public:
        explicit Replay(ReplayOptions options);
        const std::string& streamId() const;
        std::string append(const std::string& data);
        std::optional<std::string> after(const std::string& id);
        void prune(const utils::Timeval& now = utils::Timeval::currentTime());
        bool expired() const;

    private:
        struct Event {
            std::string id;
            std::string wire;
            utils::Timeval created;
        };

        ReplayOptions options;
        std::string token;
        std::deque<Event> events;
        std::uint64_t sequence = 0;
        std::size_t bytes = 0;
    };
} // namespace mcp::server::http::detail
