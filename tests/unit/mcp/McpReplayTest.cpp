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

#include "mcp/server/http/Router.h"
#include "mcp/server/http/detail/Replay.h"
#include "tests/support/TestResult.h"
#include "utils/Timeval.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <string>
#endif
int main() {
    tests::support::TestResult test;
    mcp::server::http::ReplayOptions options;
    options.maximumEvents = 2;
    options.maximumBytes = 1024;
    options.retentionMs = 10;
    mcp::server::http::detail::Replay replay(options);
    const auto eventId = [](const std::string& wire) {
        return wire.substr(4, wire.find('\n') - 4);
    };
    const auto prime = replay.append("");
    const auto first = replay.append("{\"first\":1}");
    test.expectTrue(replay.after(eventId(prime)) == first, "resume returns only events after the acknowledged id");
    const auto second = replay.append("{\"second\":2}");
    test.expectTrue(!replay.after(eventId(prime)) && replay.after(eventId(first)) == second,
                    "eviction invalidates an old id without replaying another range");
    test.expectTrue(replay.after(eventId(second)) == std::string(), "last event id yields an empty replay");
    mcp::server::http::detail::Replay other(options);
    test.expectTrue(!other.after(eventId(first)) && other.streamId() != replay.streamId(), "event ids cannot cross streams");
    replay.prune(utils::Timeval::currentTime() + utils::Timeval(1));
    test.expectTrue(replay.expired() && !replay.after(eventId(second)), "expired replay state is unavailable");
    other.append(std::string(2048, 'x'));
    test.expectTrue(other.expired(), "byte limit bounds replay storage even for an oversized event");
    return test.processResult();
}
