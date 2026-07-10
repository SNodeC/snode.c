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

#include "ConfigEditor.h"
#include "ConfigModel.h"
#include "ConfigParser.h"
#include "TestResult.h"

#include <string>

using snodec::control::applyEdits;
using snodec::control::ChangeRecord;
using snodec::control::ConfigModel;
using snodec::control::EditOperation;
using snodec::control::findOption;
using snodec::control::LookupStatus;
using snodec::control::missingRequiredOptions;
using snodec::control::parseSetArgument;
using snodec::control::parseShowConfigOutput;

namespace {

    // A small, synthetic fixture with a global option, two sections each containing a "port" key (so
    // bare "port" lookups are ambiguous), and one required option with no default.
    ConfigModel sampleModel() {
        const std::string input = "# Log level\n"
                                  "#log-level=4\n"
                                  "\n"
                                  "# Port number\n"
                                  "#echoserver.local.port=\"<REQUIRED>\"\n"
                                  "\n"
                                  "# Host name\n"
                                  "#echoserver.local.host=\"0.0.0.0\"\n"
                                  "\n"
                                  "# Other port\n"
                                  "#otherserver.local.port=9000\n";

        return parseShowConfigOutput(input).model;
    }

    void testLookupByFullKey(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();

        const auto result = findOption(model, "echoserver.local.port");
        testResult.expectTrue(result.status == LookupStatus::Found, "lookup by full key: echoserver.local.port is found");
        if (result.status == LookupStatus::Found) {
            testResult.expectEqual(std::string("port"), result.option->key, "lookup by full key: resolves to the correct option");
        }

        const auto globalResult = findOption(model, "log-level");
        testResult.expectTrue(globalResult.status == LookupStatus::Found, "lookup by full key: bare global key 'log-level' is found");
    }

    void testAmbiguousLookup(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();

        const auto result = findOption(model, "port");
        testResult.expectTrue(result.status == LookupStatus::Ambiguous, "ambiguous lookup: bare 'port' matches two sections");
        testResult.expectEqual(2, static_cast<int>(result.candidates.size()), "ambiguous lookup: exactly two candidates are listed");
        if (result.candidates.size() == 2) {
            testResult.expectEqual(std::string("echoserver.local.port"), result.candidates[0], "ambiguous lookup: first candidate");
            testResult.expectEqual(std::string("otherserver.local.port"), result.candidates[1], "ambiguous lookup: second candidate");
        }

        const auto uniqueBareResult = findOption(model, "host");
        testResult.expectTrue(uniqueBareResult.status == LookupStatus::Found, "unambiguous bare lookup: 'host' matches exactly one option");
    }

    void testUnknownLookup(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();

        const auto result = findOption(model, "does-not-exist");
        testResult.expectTrue(result.status == LookupStatus::NotFound, "unknown lookup: nonexistent key is NotFound");
    }

    void testParseSetArgument(snodec::control::test::TestResult& testResult) {
        std::string key;
        std::string value;

        testResult.expectTrue(parseSetArgument("log-level=5", key, value), "parseSetArgument: 'key=value' parses successfully");
        testResult.expectEqual(std::string("log-level"), key, "parseSetArgument: key is split correctly");
        testResult.expectEqual(std::string("5"), value, "parseSetArgument: value is split correctly");

        testResult.expectTrue(parseSetArgument("name=", key, value), "parseSetArgument: 'key=' (empty value) is valid");
        testResult.expectEqual(std::string("name"), key, "parseSetArgument: key before empty value");
        testResult.expectEqual(std::string(""), value, "parseSetArgument: value is empty, not absent");

        testResult.expectTrue(parseSetArgument("some.path=/tmp/a b", key, value),
                              "parseSetArgument: value may contain spaces (argv, not shell)");
        testResult.expectEqual(std::string("/tmp/a b"), value, "parseSetArgument: value with a space is preserved verbatim");

        testResult.expectTrue(parseSetArgument("echoserver.local.host=1.2.3.4=5", key, value),
                              "parseSetArgument: only the first '=' splits key/value");
        testResult.expectEqual(std::string("1.2.3.4=5"), value, "parseSetArgument: a value containing '=' is kept whole");

        testResult.expectTrue(!parseSetArgument("no-equals-sign", key, value), "parseSetArgument: malformed input with no '=' is rejected");
        testResult.expectTrue(!parseSetArgument("=value", key, value), "parseSetArgument: malformed input with an empty key is rejected");
    }

    void testSetKnownOption(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();

        const auto outcome = applyEdits(model, {EditOperation{EditOperation::Kind::Set, "echoserver.local.port", "8080"}}, false);

        testResult.expectTrue(outcome.errors.empty(), "--set known option: no errors");
        testResult.expectEqual(1, static_cast<int>(outcome.changes.size()), "--set known option: exactly one change recorded");
        if (!outcome.changes.empty()) {
            testResult.expectEqual(std::string("echoserver.local.port"), outcome.changes[0].fullKey, "--set: change full key");
            testResult.expectEqual(std::string("<REQUIRED>"), outcome.changes[0].before, "--set: 'before' shows <REQUIRED>");
            testResult.expectEqual(std::string("8080"), outcome.changes[0].after, "--set: 'after' shows the new value");
        }

        const auto lookup = findOption(model, "echoserver.local.port");
        testResult.expectTrue(lookup.status == LookupStatus::Found && lookup.option->hasActiveValue &&
                                  *lookup.option->activeValue == "8080",
                              "--set known option: the model itself is mutated");
    }

    void testSetEmptyValue(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();

        // --set key= with --allow-new-options exercises both "empty value" and "new option creation".
        const auto outcome = applyEdits(model, {EditOperation{EditOperation::Kind::Set, "custom.name", ""}}, true);

        testResult.expectTrue(outcome.errors.empty(), "--set key= with --allow-new-options: no errors");
        testResult.expectEqual(1, static_cast<int>(outcome.changes.size()), "--set key=: a change is recorded for the new option");
        if (!outcome.changes.empty()) {
            testResult.expectEqual(std::string("<unset>"), outcome.changes[0].before, "--set key=: 'before' a new option is <unset>");
            testResult.expectEqual(std::string("\"\""), outcome.changes[0].after, "--set key=: empty value displays as \"\"");
        }
    }

    void testMalformedSetIsRejectedByCli(snodec::control::test::TestResult& testResult) {
        // The Cli layer rejects a malformed --set before ever calling applyEdits; here we simply confirm
        // the primitive parseSetArgument() the Cli relies on reports failure for such input.
        std::string key;
        std::string value;
        testResult.expectTrue(!parseSetArgument("just-a-key-no-value", key, value), "malformed --set: rejected without an '='");
    }

    void testUnknownEditKeyIsAnError(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();

        const auto outcome = applyEdits(model, {EditOperation{EditOperation::Kind::Set, "does.not.exist", "1"}}, false);

        testResult.expectTrue(outcome.changes.empty(), "unknown edit key: no changes recorded");
        testResult.expectEqual(1, static_cast<int>(outcome.errors.size()), "unknown edit key: exactly one error recorded");
    }

    void testAmbiguousEditKeyIsAnError(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();

        const auto outcome = applyEdits(model, {EditOperation{EditOperation::Kind::Set, "port", "1"}}, false);

        testResult.expectTrue(outcome.changes.empty(), "ambiguous edit key: no changes recorded");
        testResult.expectEqual(1, static_cast<int>(outcome.errors.size()), "ambiguous edit key: exactly one error recorded");
    }

    void testUnset(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();

        // First give log-level an active value, then unset it again.
        const auto setOutcome = applyEdits(model, {EditOperation{EditOperation::Kind::Set, "log-level", "5"}}, false);
        testResult.expectEqual(1, static_cast<int>(setOutcome.changes.size()), "--unset setup: the initial --set is recorded");

        const auto unsetOutcome = applyEdits(model, {EditOperation{EditOperation::Kind::Unset, "log-level", ""}}, false);
        testResult.expectTrue(unsetOutcome.errors.empty(), "--unset: no errors");
        testResult.expectEqual(1, static_cast<int>(unsetOutcome.changes.size()), "--unset: a change is recorded");
        if (!unsetOutcome.changes.empty()) {
            testResult.expectEqual(std::string("5"), unsetOutcome.changes[0].before, "--unset: 'before' shows the active value");
            testResult.expectEqual(std::string("<unset>"), unsetOutcome.changes[0].after, "--unset: 'after' is the literal <unset>");
        }

        const auto lookup = findOption(model, "log-level");
        testResult.expectTrue(lookup.status == LookupStatus::Found && !lookup.option->hasActiveValue,
                              "--unset: the active value is actually cleared");
        testResult.expectTrue(lookup.option->defaultValue.has_value() && *lookup.option->defaultValue == "4",
                              "--unset: the default value is preserved");

        // Unsetting an option that never had an active value is a no-op: no change, no error.
        const auto noopOutcome = applyEdits(model, {EditOperation{EditOperation::Kind::Unset, "echoserver.local.host", ""}}, false);
        testResult.expectTrue(noopOutcome.errors.empty(), "--unset no-op: no errors");
        testResult.expectTrue(noopOutcome.changes.empty(), "--unset no-op: unsetting an already-inactive option records no change");
    }

    void testCheckRequired(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();

        auto missingBefore = missingRequiredOptions(model);
        testResult.expectEqual(1, static_cast<int>(missingBefore.size()), "check-required: port is missing before any edits");

        applyEdits(model, {EditOperation{EditOperation::Kind::Set, "echoserver.local.port", "8080"}}, false);

        auto missingAfter = missingRequiredOptions(model);
        testResult.expectTrue(missingAfter.empty(), "check-required: no options are missing once the required port is set");
    }

    void testDiffNoChanges(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();

        // Setting a value identical to its current effective display should not be reported as a change.
        const auto outcome = applyEdits(model, {EditOperation{EditOperation::Kind::Set, "log-level", "4"}}, false);
        testResult.expectTrue(outcome.errors.empty(), "diff no-op: setting the existing effective value is not an error");
        testResult.expectTrue(outcome.changes.empty(), "diff no-op: setting the existing effective value produces no change");
    }

} // namespace

int main() {
    snodec::control::test::TestResult testResult;

    testLookupByFullKey(testResult);
    testAmbiguousLookup(testResult);
    testUnknownLookup(testResult);
    testParseSetArgument(testResult);
    testSetKnownOption(testResult);
    testSetEmptyValue(testResult);
    testMalformedSetIsRejectedByCli(testResult);
    testUnknownEditKeyIsAnError(testResult);
    testAmbiguousEditKeyIsAnError(testResult);
    testUnset(testResult);
    testCheckRequired(testResult);
    testDiffNoChanges(testResult);

    return testResult.processResult();
}
