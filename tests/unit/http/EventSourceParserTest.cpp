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

#include "tests/support/TestResult.h"
#include "web/http/client/tools/EventSource.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <string>
#include <vector>
#endif
namespace {
    class Parser final : public web::http::client::tools::EventSource {
    public:
        bool consume(std::string_view bytes) {
            return feed(sharedState, bytes);
        }

        void close() final {
        }
    };
} // namespace

int main() {
    tests::support::TestResult test;
    Parser parser;
    std::vector<web::http::client::tools::EventSource::MessageEvent> events;
    parser.onMessage([&events](const auto& event) {
        events.push_back(event);
    });
    const std::string wire =
        "\xEF\xBB\xBF: comment\r\nid: first\rdata: one\r\ndata: two\n\nretry: 7\nid: second\r\n\r\ndata:\r\rid:\n\ndata: last\n\n";
    bool valid = true;
    for (const char byte : wire) {
        valid = parser.consume(std::string_view(&byte, 1)) && valid;
    }
    test.expectTrue(valid && events.size() == 3, "fragmented UTF-8 BOM, comments, CR, LF and CRLF delimit events");
    if (events.size() == 3) {
        test.expectTrue(events[0].data == "one\ntwo" && events[0].lastEventId == "first", "data lines join and id is retained");
        test.expectTrue(events[1].data.empty() && events[1].lastEventId == "second", "empty-data priming event records replay id");
        test.expectTrue(events[2].data == "last" && events[2].lastEventId.empty(), "empty id resets reconnect header");
    }
    test.expectTrue(parser.retry() == 7, "retry directive is shared with transport policy");
    parser.consume("retry: 999999999999999999999999999999\n\n");
    test.expectTrue(parser.retry() == 0xFFFFFFFFU, "retry overflow saturates");
    test.expectTrue(!parser.consume(std::string(256 * 1024 + 1, 'x')), "partial-line storage is bounded");
    Parser data;
    bool bounded = true;
    for (unsigned index = 0; index < 5; ++index) {
        bounded = data.consume("data: " + std::string(250 * 1024, 'x') + "\n") && bounded;
    }
    test.expectTrue(!bounded, "coalesced multiline payload storage is bounded");
    return test.processResult();
}
