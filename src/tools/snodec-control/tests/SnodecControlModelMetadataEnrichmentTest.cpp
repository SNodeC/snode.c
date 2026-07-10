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
#include "Metadata.h"
#include "TestResult.h"
#include "fixtures/NestedSubCommandFixture.h"
#include "ui/UiState.h"

#include <algorithm>
#include <string>

using snodec::control::applyMetadataToModel;
using snodec::control::ConfigModel;
using snodec::control::ConfigOption;
using snodec::control::parseMetaBlocks;
using snodec::control::parseShowConfigOutput;
using snodec::control::ParsedMetadata;

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

    void testApplyMetadataEnrichesRealCapturedOptions(snodec::control::test::TestResult& testResult) {
        ConfigModel model = parseShowConfigOutput(snodec::control::test::fixtures::nestedSubCommandShowConfigOutput).model;
        const ParsedMetadata metadata = parseMetaBlocks(snodec::control::test::fixtures::nestedSubCommandShowConfigOutput);

        applyMetadataToModel(model, metadata);

        const ConfigOption* logLevel = findOption(model, "", "log-level");
        testResult.expectTrue(logLevel != nullptr, "enrichment: log-level option found");
        if (logLevel != nullptr) {
            testResult.expectTrue(logLevel->typeKind.has_value(), "enrichment: log-level got a typeKind from metadata");
            testResult.expectTrue(logLevel->requiredEffective.has_value(), "enrichment: log-level got a requiredEffective from metadata");
            testResult.expectTrue(logLevel->configFileWritable.has_value() && *logLevel->configFileWritable,
                                  "enrichment: log-level's configFile.writable decodes to true");
        }

        // --token needs --host and excludes --port in the ConfigFormatterCommentMetadataTest-style
        // synthetic app; this fixture doesn't define needs/excludes, so instead confirm the *shape* is
        // present-but-empty rather than crashing/misbehaving on an option with no relations.
        if (logLevel != nullptr) {
            testResult.expectTrue(logLevel->needs.empty(), "enrichment: log-level has no needs (none declared)");
            testResult.expectTrue(logLevel->excludes.empty(), "enrichment: log-level has no excludes (none declared)");
        }
    }

    void testApplyMetadataIsNoOpWhenMetadataAbsent(snodec::control::test::TestResult& testResult) {
        const std::string input = "# Log level\n"
                                  "#log-level=4\n"
                                  "log-level=4\n";
        ConfigModel model = parseShowConfigOutput(input).model;
        const ParsedMetadata emptyMetadata;

        applyMetadataToModel(model, emptyMetadata);

        const ConfigOption* logLevel = findOption(model, "", "log-level");
        testResult.expectTrue(logLevel != nullptr, "no-op: log-level option found");
        if (logLevel != nullptr) {
            testResult.expectTrue(!logLevel->typeKind.has_value(), "no-op: absent metadata leaves typeKind unset");
            testResult.expectTrue(!logLevel->requiredEffective.has_value(), "no-op: absent metadata leaves requiredEffective unset");
        }
    }

    // Step 4's list-awareness guard: a multi-value ("type.items": "list") option must never be
    // cycled by Space as if it were a scalar boolean/tristate, even if one of its space-joined values
    // happens to read "true"/"false"/"default".
    void testListTypeOptionIsNeverBooleanOrTristateLike(snodec::control::test::TestResult& testResult) {
        ConfigOption option;
        option.key = "flags";
        option.defaultValue = "default";
        option.hasActiveValue = true;
        option.activeValue = "true false";
        option.typeItems = "list";

        testResult.expectTrue(!snodec::control::ui::isTristateLikeOption(option), "list guard: a list-type option is never tristate-like, even with default == \"default\"");
        testResult.expectTrue(!snodec::control::ui::isBooleanLikeOption(option), "list guard: a list-type option is never boolean-like");
        testResult.expectTrue(!snodec::control::ui::nextCycledValue(option).has_value(), "list guard: Space produces no cycled value for a list-type option");
    }

    void testScalarBooleanOptionIsUnaffectedByListGuard(snodec::control::test::TestResult& testResult) {
        ConfigOption option;
        option.key = "verbose";
        option.defaultValue = "false";
        option.typeItems = "single";

        testResult.expectTrue(snodec::control::ui::isBooleanLikeOption(option), "list guard: a single-item option is unaffected and still boolean-like");
    }

} // namespace

int main() {
    snodec::control::test::TestResult testResult;

    testApplyMetadataEnrichesRealCapturedOptions(testResult);
    testApplyMetadataIsNoOpWhenMetadataAbsent(testResult);
    testListTypeOptionIsNeverBooleanOrTristateLike(testResult);
    testScalarBooleanOptionIsUnaffectedByListGuard(testResult);

    return testResult.processResult();
}
