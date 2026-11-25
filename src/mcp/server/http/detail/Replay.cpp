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

#include "mcp/server/http/detail/Replay.h"

#include "mcp/detail/Utilities.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <algorithm>
#include <cstdint>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string>
#endif
namespace mcp::server::http::detail {
    Replay::Replay(ReplayOptions options)
        : options(options)
        , token(mcp::detail::randomToken()) {
    }

    const std::string& Replay::streamId() const {
        return token;
    }

    std::string Replay::append(const std::string& data) {
        if (sequence == UINT64_MAX) {
            throw std::runtime_error("SSE sequence exhausted");
        }
        const auto now = utils::Timeval::currentTime();
        const std::string id = token + "." + std::to_string(++sequence);
        const std::string wire = "id: " + id + "\nevent: message\n" + (data.empty() ? "data:\n\n" : "data: " + data + "\n\n");
        events.push_back(Event{id, wire, now});
        bytes += wire.size();
        prune(now);
        return wire;
    }

    void Replay::prune(const utils::Timeval& now) {
        while (!events.empty() && (events.size() > options.maximumEvents || bytes > options.maximumBytes ||
                                   events.front().created + utils::Timeval(static_cast<double>(options.retentionMs) / 1000.0) <= now)) {
            bytes -= events.front().wire.size();
            events.pop_front();
        }
    }

    bool Replay::expired() const {
        return events.empty();
    }

    std::optional<std::string> Replay::after(const std::string& id) {
        prune();
        std::optional<std::string> wire;
        const auto found = std::find_if(events.begin(), events.end(), [&id](const auto& event) {
            return event.id == id;
        });
        if (found != events.end()) {
            wire = std::string();
            for (auto event = std::next(found); event != events.end(); ++event) {
                *wire += event->wire;
            }
        }
        return wire;
    }
} // namespace mcp::server::http::detail
