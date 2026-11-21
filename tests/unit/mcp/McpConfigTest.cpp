// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "mcp/server/Config.h"
#include "tests/support/TestResult.h"
#include <cmath>
#include <functional>
#include <sstream>
#include <stdexcept>

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
                        config.getDeadline().getMsd() == 30000 && config.getAllowedOrigins().empty(),
                    "unchanged CLI defaults");
    test.expectTrue(config.setMaximumMessageBytes(1024)
                            ->setMaximumOutputBytes(4096)
                            ->setDeadline(utils::Timeval(1.25))
                            ->setAllowedOrigins({"https://one.test"}) == &config,
                    "chainable typed defaults");
    test.expectTrue(config.getMaximumMessageBytes() == 1024 && config.getMaximumOutputBytes() == 4096 &&
                        config.getDeadline().getMsd() == 1250 && config.getAllowedOrigins() == std::vector<std::string>{"https://one.test"},
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
