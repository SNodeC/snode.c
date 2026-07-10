/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
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

#include "ConfigModel.h"
#include "ConfigParser.h"
#include "TestResult.h"
#include "fixtures/NestedSubCommandFixture.h"

#include <algorithm>
#include <string>

using snodec::control::ConfigModel;
using snodec::control::ConfigOption;
using snodec::control::ConfigSection;
using snodec::control::parseShowConfigOutput;

namespace {

    const ConfigOption* findOption(const ConfigModel& model, const std::string& section, const std::string& key) {
        for (const auto& configSection : model.sections) {
            if (configSection.name == section) {
                auto it = std::find_if(configSection.options.begin(), configSection.options.end(), [&key](const ConfigOption& option) {
                    return option.key == key;
                });
                if (it != configSection.options.end()) {
                    return &(*it);
                }
            }
        }
        return nullptr;
    }

    // Covers: global (unsectioned) options, a commented default-only option, description comments,
    // an empty string default value, and boolean-looking values.
    void testGlobalOptionsAndDescriptions(snodec::control::test::TestResult& testResult) {
        const std::string input = "### Configuration for Application 'sample-app'\n"
                                  "## Options (persistent)\n"
                                  "# Log level\n"
                                  "#log-level=4\n"
                                  "\n"
                                  "# Log file\n"
                                  "#log-file=\"\"\n"
                                  "\n"
                                  "# Monochrom log output\n"
                                  "#monochrom=true\n"
                                  "\n"
                                  "# Quiet mode\n"
                                  "#quiet=false\n";

        const auto parseResult = parseShowConfigOutput(input);

        testResult.expectTrue(parseResult.warnings.empty(), "global options: no warnings expected");
        testResult.expectEqual(1, static_cast<int>(parseResult.model.sectionCount()), "global options: exactly one (root) section");

        const ConfigOption* logLevel = findOption(parseResult.model, "", "log-level");
        testResult.expectTrue(logLevel != nullptr, "global options: log-level option is found in the root section");
        if (logLevel != nullptr) {
            testResult.expectTrue(logLevel->description == "Log level", "global options: description comment is attached to log-level");
            testResult.expectTrue(logLevel->defaultValue.has_value() && *logLevel->defaultValue == "4",
                                  "commented default-only option: log-level keeps its default and has no active value");
            testResult.expectTrue(!logLevel->hasActiveValue, "commented default-only option: log-level has no active value");
        }

        const ConfigOption* logFile = findOption(parseResult.model, "", "log-file");
        testResult.expectTrue(logFile != nullptr, "empty string value: log-file option is found");
        if (logFile != nullptr) {
            testResult.expectTrue(logFile->defaultValue.has_value() && logFile->defaultValue->empty(),
                                  "empty string value: log-file default is present and empty, not absent");
        }

        const ConfigOption* monochrom = findOption(parseResult.model, "", "monochrom");
        testResult.expectTrue(monochrom != nullptr, "boolean-looking values: monochrom option is found");
        if (monochrom != nullptr) {
            testResult.expectTrue(monochrom->defaultValue.has_value() && *monochrom->defaultValue == "true",
                                  "boolean-looking values: monochrom default is the literal token 'true'");
        }

        const ConfigOption* quiet = findOption(parseResult.model, "", "quiet");
        testResult.expectTrue(quiet != nullptr, "boolean-looking values: quiet option is found");
        if (quiet != nullptr) {
            testResult.expectTrue(quiet->defaultValue.has_value() && *quiet->defaultValue == "false",
                                  "boolean-looking values: quiet default is the literal token 'false'");
        }
    }

    // Covers: a named, dot-derived section, a required placeholder default, and an active value
    // overriding a default.
    void testNamedSectionRequiredAndActiveOverride(snodec::control::test::TestResult& testResult) {
        const std::string input = "### Configuration for server instance 'myserver'\n"
                                  "## Options (persistent)\n"
                                  "# Disable this instance\n"
                                  "#myserver.disabled=false\n"
                                  "\n"
                                  "### Local side of connection for instance 'myserver'\n"
                                  "## Options (persistent)\n"
                                  "# Port number\n"
                                  "#myserver.local.port=\"<REQUIRED>\"\n"
                                  "myserver.local.port=9000\n";

        const auto parseResult = parseShowConfigOutput(input);

        testResult.expectTrue(parseResult.warnings.empty(), "named section: no warnings expected");

        const ConfigOption* disabled = findOption(parseResult.model, "myserver", "disabled");
        testResult.expectTrue(disabled != nullptr, "one named section: 'myserver' section holds 'disabled'");

        const ConfigOption* port = findOption(parseResult.model, "myserver.local", "port");
        testResult.expectTrue(port != nullptr, "one named section: 'myserver.local' sub-section holds 'port'");
        if (port != nullptr) {
            testResult.expectTrue(port->required, "required placeholder: <REQUIRED> default marks the option as required");
            testResult.expectTrue(!port->defaultValue.has_value(),
                                  "required placeholder: no usable default value is stored for a <REQUIRED> option");
            testResult.expectTrue(port->hasActiveValue && port->activeValue.has_value() && *port->activeValue == "9000",
                                  "active value overriding default: the active value 9000 is captured");
        }
    }

    // Covers explicit `[section]` bracket headers, which Phase 1 must support even though SNode.C's own
    // `-s` output derives sections from dotted option names instead.
    void testBracketSectionHeader(snodec::control::test::TestResult& testResult) {
        const std::string input = "[explicit-section]\n"
                                  "# An explicitly bracketed flag\n"
                                  "#flag=false\n"
                                  "flag=true\n";

        const auto parseResult = parseShowConfigOutput(input);

        testResult.expectTrue(parseResult.warnings.empty(), "bracket section: no warnings expected");

        const ConfigOption* flag = findOption(parseResult.model, "explicit-section", "flag");
        testResult.expectTrue(flag != nullptr, "bracket section: '[explicit-section]' header creates the expected section");
        if (flag != nullptr) {
            testResult.expectTrue(flag->defaultValue.has_value() && *flag->defaultValue == "false",
                                  "bracket section: commented default value is 'false'");
            testResult.expectTrue(flag->hasActiveValue && flag->activeValue.has_value() && *flag->activeValue == "true",
                                  "bracket section: active value 'true' overrides the default");
        }
    }

    // A malformed line must not abort parsing: it is recorded as a warning and the rest of the input is
    // still parsed normally.
    void testMalformedLineIsRecoverable(snodec::control::test::TestResult& testResult) {
        const std::string input = "[RequiresError] some-app requires an instance\n"
                                  "\n"
                                  "# Log level\n"
                                  "log-level=4\n";

        const auto parseResult = parseShowConfigOutput(input);

        testResult.expectEqual(1, static_cast<int>(parseResult.warnings.size()), "malformed config lines: exactly one warning is recorded");
        testResult.expectEqual(
            1, static_cast<int>(parseResult.model.optionCount()), "malformed config lines: parsing continues after the bad line");
    }

    // Regression test for the "#@ snodec.meta ..." structured-metadata comment blocks that real
    // SNode.C targets now emit ahead of a node's/group's/option's human-readable description and
    // config/default lines (see docs/config-comment-metadata.md in the SNode.C tree). Before the
    // fix, every "#@ ..." line fell through to the generic "# ..." comment-description-accumulation
    // branch and got folded into the *following* option's description, so e.g. a deeply nested
    // option's description would start with "snodec.meta begin option" and a full pretty-printed
    // JSON blob rather than its real one-line description. Uses real captured `-s` output (not a
    // hand-written snippet) so this exercises the framework's actual emission order.
    void testMetadataCommentsDoNotPolluteDescriptions(snodec::control::test::TestResult& testResult) {
        const auto parseResult = parseShowConfigOutput(snodec::control::test::fixtures::nestedSubCommandShowConfigOutput);

        const ConfigOption* deepOption = findOption(parseResult.model, "echoserver.outer.inner", "depth-value");
        testResult.expectTrue(deepOption != nullptr, "metadata comments: deeply nested option is still discovered");
        if (deepOption != nullptr) {
            testResult.expectEqual(
                std::string("A deeply nested option"), deepOption->description, "metadata comments: deeply nested option's description is clean, not polluted with JSON");
        }

        const ConfigOption* outerOption = findOption(parseResult.model, "echoserver.outer", "outer-value");
        testResult.expectTrue(outerOption != nullptr, "metadata comments: outer option is still discovered");
        if (outerOption != nullptr) {
            testResult.expectEqual(
                std::string("An outer option"), outerOption->description, "metadata comments: outer option's description is clean");
        }

        const ConfigOption* toolOption = findOption(parseResult.model, "tool", "tool-value");
        testResult.expectTrue(toolOption != nullptr, "metadata comments: top-level custom subcommand option is still discovered");
        if (toolOption != nullptr) {
            testResult.expectEqual(std::string("A top-level tool option"), toolOption->description, "metadata comments: top-level tool option's description is clean");
        }

        for (const ConfigSection& section : parseResult.model.sections) {
            for (const ConfigOption& option : section.options) {
                testResult.expectTrue(option.description.find("snodec.meta") == std::string::npos,
                                      "metadata comments: no option description contains a leaked 'snodec.meta' marker (" + section.name +
                                          "." + option.key + ")");
                testResult.expectTrue(option.description.find("\"schema\"") == std::string::npos,
                                      "metadata comments: no option description contains leaked JSON (" + section.name + "." + option.key +
                                          ")");
            }
        }
    }

} // namespace

int main() {
    snodec::control::test::TestResult testResult;

    testGlobalOptionsAndDescriptions(testResult);
    testNamedSectionRequiredAndActiveOverride(testResult);
    testBracketSectionHeader(testResult);
    testMalformedLineIsRecoverable(testResult);
    testMetadataCommentsDoNotPolluteDescriptions(testResult);

    return testResult.processResult();
}
