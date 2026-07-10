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

#include "CommandBuilder.h"
#include "TestResult.h"

#include <optional>
#include <string>
#include <vector>

using snodec::control::buildDiscoveryArgs;
using snodec::control::buildRunArgs;
using snodec::control::buildSaveArgs;
using snodec::control::containsConfigFileFlag;
using snodec::control::formatCommandForDisplay;
using snodec::control::shellEscape;

namespace {

    void testShellEscape(snodec::control::test::TestResult& testResult) {
        testResult.expectEqual(
            std::string("plain-value_1.2:3/4"), shellEscape("plain-value_1.2:3/4"), "shellEscape: a 'safe' argument is left unquoted");
        testResult.expectEqual(std::string("''"), shellEscape(""), "shellEscape: an empty argument is quoted as ''");
        testResult.expectEqual(std::string("'a b'"), shellEscape("a b"), "shellEscape: a value with a space is single-quoted");
        testResult.expectEqual(
            std::string("'it'\\''s'"), shellEscape("it's"), "shellEscape: an embedded single quote is escaped correctly");
    }

    void testFormatCommandForDisplay(snodec::control::test::TestResult& testResult) {
        const std::string command = formatCommandForDisplay("/path/to/app", {"--config-file", "/tmp/a b.conf"});
        testResult.expectEqual(std::string("/path/to/app --config-file '/tmp/a b.conf'"),
                               command,
                               "formatCommandForDisplay: executable and arguments are joined and escaped");
    }

    void testContainsConfigFileFlag(snodec::control::test::TestResult& testResult) {
        testResult.expectTrue(containsConfigFileFlag({"--config-file", "/tmp/x.conf"}), "containsConfigFileFlag: long form is detected");
        testResult.expectTrue(containsConfigFileFlag({"-c", "/tmp/x.conf"}), "containsConfigFileFlag: short form is detected");
        testResult.expectTrue(containsConfigFileFlag({"--config-file=/tmp/x.conf"}), "containsConfigFileFlag: inline '=' form is detected");
        testResult.expectTrue(!containsConfigFileFlag({"--log-level", "5"}), "containsConfigFileFlag: unrelated args are not flagged");
    }

    void testDiscoveryArgs(snodec::control::test::TestResult& testResult) {
        const auto args = buildDiscoveryArgs({"--log-level", "5"});
        const std::vector<std::string> expected{"--log-level", "5", "-s"};
        testResult.expectTrue(args == expected, "buildDiscoveryArgs: target-args are followed by '-s'");
    }

    void testSaveArgsConstruction(snodec::control::test::TestResult& testResult) {
        const auto args = buildSaveArgs({"--log-level", "5"}, "/tmp/temp.conf", "/tmp/out.conf");
        testResult.expectTrue(args.has_value(), "buildSaveArgs: succeeds when target-args has no config-file flag");
        if (args.has_value()) {
            const std::vector<std::string> expected{
                "--log-level", "5", "--config-file", "/tmp/temp.conf", "--write-config", "/tmp/out.conf"};
            testResult.expectTrue(*args == expected, "buildSaveArgs: argv is target-args + --config-file + --write-config");
        }

        const auto conflicting = buildSaveArgs({"--config-file", "/tmp/existing.conf"}, "/tmp/temp.conf", "/tmp/out.conf");
        testResult.expectTrue(!conflicting.has_value(), "buildSaveArgs: conflicts when target-args already has a config-file flag");
    }

    void testRunArgsConstruction(snodec::control::test::TestResult& testResult) {
        const auto argsWithConfig = buildRunArgs({"--log-level", "5"}, std::optional<std::string>("/tmp/run.conf"));
        testResult.expectTrue(argsWithConfig.has_value(), "buildRunArgs: succeeds with a config path and no conflict");
        if (argsWithConfig.has_value()) {
            const std::vector<std::string> expected{"--log-level", "5", "--config-file", "/tmp/run.conf"};
            testResult.expectTrue(*argsWithConfig == expected, "buildRunArgs: argv is target-args + --config-file <path>");
        }

        const auto argsWithoutConfig = buildRunArgs({"--log-level", "5"}, std::nullopt);
        testResult.expectTrue(argsWithoutConfig.has_value(), "buildRunArgs: succeeds with no config path");
        if (argsWithoutConfig.has_value()) {
            const std::vector<std::string> expected{"--log-level", "5"};
            testResult.expectTrue(*argsWithoutConfig == expected, "buildRunArgs: with no config path, argv is just target-args");
        }

        const auto conflicting = buildRunArgs({"-c", "/tmp/existing.conf"}, std::optional<std::string>("/tmp/run.conf"));
        testResult.expectTrue(!conflicting.has_value(), "buildRunArgs: conflicts when target-args already has a config-file flag");

        // No conflict when target-args has a config-file flag but we are not adding our own.
        const auto noConfigNoConflict = buildRunArgs({"-c", "/tmp/existing.conf"}, std::nullopt);
        testResult.expectTrue(noConfigNoConflict.has_value(),
                              "buildRunArgs: target-args' own config-file flag is fine when we add none of our own");
    }

} // namespace

int main() {
    snodec::control::test::TestResult testResult;

    testShellEscape(testResult);
    testFormatCommandForDisplay(testResult);
    testContainsConfigFileFlag(testResult);
    testDiscoveryArgs(testResult);
    testSaveArgsConstruction(testResult);
    testRunArgsConstruction(testResult);

    return testResult.processResult();
}
