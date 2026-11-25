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

#include "mcp/server/Config.h"
#include "tests/support/TestResult.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cmath>
#include <functional>
#include <sstream>
#include <stdexcept>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace {

    class Root : public utils::SubCommand {
    public:
        Root()
            : SubCommand(nullptr, std::make_shared<utils::AppWithPtr>("test root", "root", this), "Tests") {
        }
    };

} // namespace

int main() {
    tests::support::TestResult test;
    Root root;
    mcp::server::Config config(&root);
    test.expectTrue(config.getMaximumMessageBytes() == 1048576 && config.getMaximumOutputBytes() == 1048576 &&
                        config.getDeadline().getMsd() == 30000 && config.getPageSize() == 128 && config.getAllowedOrigins().empty(),
                    "unchanged CLI defaults");
    test.expectTrue(config.setMaximumMessageBytes(1024)
                            ->setMaximumOutputBytes(4096)
                            ->setDeadline(utils::Timeval(1.25))
                            ->setPageSize(2)
                            ->setAllowedOrigins({"https://one.test"}) == &config,
                    "chainable typed defaults");
    test.expectTrue(config.getMaximumMessageBytes() == 1024 && config.getMaximumOutputBytes() == 4096 &&
                        config.getDeadline().getMsd() == 1250 && config.getPageSize() == 2 &&
                        config.getAllowedOrigins() == std::vector<std::string>{"https://one.test"},
                    "setters update effective defaults before parse");
    const auto invalid = [&](const std::function<void()>& action) {
        bool rejected = false;
        try {
            action();
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        test.expectTrue(rejected, "invalid setter default rejected by the existing option validator");
    };
    invalid([&] {
        config.setPageSize(0);
    });
    invalid([&] {
        config.setPageSize(129);
    });
    invalid([&] {
        config.setMaximumMessageBytes(0);
    });
    invalid([&] {
        config.setMaximumMessageBytes(16777217);
    });
    invalid([&] {
        config.setMaximumOutputBytes(1023);
    });
    invalid([&] {
        config.setDeadline(utils::Timeval(0));
    });
    invalid([&] {
        config.setDeadline(utils::Timeval(301));
    });
    test.expectTrue(config.getMaximumMessageBytes() == 1024 && config.getMaximumOutputBytes() == 4096 &&
                        config.getDeadline().getMsd() == 1250,
                    "rejected defaults preserve previous settings");
    config.setAllowedOrigins({});
    test.expectTrue(config.getAllowedOrigins().empty(), "empty API Origin list remains empty");
    const auto* origins = &config.getAllowedOrigins();
    root.getAppWithPtr()->parse("mcp --maximum-message-bytes 2048 --maximum-output-bytes 8192 --deadline-ms 2500 --allowed-origin "
                                "https://two.test --allowed-origin https://three.test");
    test.expectTrue(config.getMaximumMessageBytes() == 2048 && config.getMaximumOutputBytes() == 8192 &&
                        config.getDeadline().getMsd() == 2500,
                    "CLI values override API defaults");
    test.expectTrue(origins == &config.getAllowedOrigins() &&
                        *origins == std::vector<std::string>{"https://two.test", "https://three.test"},
                    "repeated origins update one stable bound vector");
    const std::string exported = root.configToStr();
    test.expectTrue(exported.find("deadline-ms") != std::string::npos && exported.find("2500") != std::string::npos,
                    "show-config retains deadline option spelling and millisecond units");
    Root restored;
    mcp::server::Config copy(&restored);
    std::istringstream input(exported);
    restored.getAppWithPtr()->parse_from_stream(input);
    test.expectTrue(copy.getMaximumMessageBytes() == 2048 && copy.getMaximumOutputBytes() == 8192 && copy.getDeadline().getMsd() == 2500 &&
                        copy.getAllowedOrigins() == *origins,
                    "configuration output round trip");
    root.getAppWithPtr()->clear();
    root.getAppWithPtr()->parse("mcp --allowed-origin https://changed.test --deadline-ms 30.5");
    test.expectTrue(origins == &config.getAllowedOrigins() && *origins == std::vector<std::string>{"https://changed.test"},
                    "reparse changes authoritative origins without a router cache");
    test.expectTrue(std::abs(config.getDeadline().getMsd() - 30.5) < 0.002, "Timeval retains fractional milliseconds");
    root.getAppWithPtr()->clear();
    root.getAppWithPtr()->parse("mcp");
    test.expectTrue(config.getAllowedOrigins().empty(), "removing Origin options on reparse restores the empty default");
    for (const std::string value : {"nan", "NaN", "nan(payload)", "inf", "abc", "0", "300001"}) {
        bool rejected = false;
        root.getAppWithPtr()->clear();
        try {
            root.getAppWithPtr()->parse("mcp --deadline-ms " + value);
        } catch (const CLI::ParseError&) {
            rejected = true;
        }
        test.expectTrue(rejected, "deadline remains finite and bounded: " + value);
    }

    return test.processResult();
}
