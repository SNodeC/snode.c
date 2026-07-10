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

#include "Json.h"
#include "Metadata.h"
#include "TestResult.h"
#include "fixtures/NestedSubCommandFixture.h"

#include <algorithm>
#include <string>

using snodec::control::JsonValue;
using snodec::control::MetaGroup;
using snodec::control::MetaNode;
using snodec::control::MetaOption;
using snodec::control::parseJson;
using snodec::control::parseMetaBlocks;
using snodec::control::ParsedMetadata;

namespace {

    const MetaNode* findNode(const ParsedMetadata& metadata, const std::vector<std::string>& path) {
        auto it = std::find_if(metadata.nodes.begin(), metadata.nodes.end(), [&path](const MetaNode& node) {
            return node.path == path;
        });
        return it != metadata.nodes.end() ? &(*it) : nullptr;
    }

    const MetaOption* findOption(const ParsedMetadata& metadata, const std::string& key) {
        auto it = std::find_if(metadata.options.begin(), metadata.options.end(), [&key](const MetaOption& option) {
            return option.key == key;
        });
        return it != metadata.options.end() ? &(*it) : nullptr;
    }

    void testJsonParserRoundTripsScalarsAndEscapes(snodec::control::test::TestResult& testResult) {
        const auto result = parseJson(R"({"a": "hi\n\"there\"", "b": true, "c": null, "d": 42, "e": ["x", "y"]})");

        testResult.expectTrue(result.ok, "json: well-formed object with escapes/array parses successfully");
        testResult.expectEqual(std::string("hi\n\"there\""), result.value.find("a")->asString(), "json: string escapes decode correctly");
        testResult.expectTrue(result.value.find("b")->asBool(false), "json: boolean true decodes correctly");
        testResult.expectTrue(result.value.find("c")->isNull(), "json: null decodes correctly");
        testResult.expectEqual(42, static_cast<int>(result.value.find("d")->asNumber(0)), "json: integer decodes correctly");
        const auto array = result.value.find("e")->asStringArray();
        testResult.expectEqual(2, static_cast<int>(array.size()), "json: string array has expected length");
        if (array.size() == 2) {
            testResult.expectEqual(std::string("x"), array[0], "json: string array first element decodes correctly");
            testResult.expectEqual(std::string("y"), array[1], "json: string array second element decodes correctly");
        }
    }

    void testJsonParserRejectsMalformedInput(snodec::control::test::TestResult& testResult) {
        testResult.expectTrue(!parseJson(R"({"a": )").ok, "json: truncated object is rejected");
        testResult.expectTrue(!parseJson(R"({"a": 1,})").ok, "json: trailing comma is rejected");
        testResult.expectTrue(!parseJson(R"({"a" 1})").ok, "json: missing colon is rejected");
        testResult.expectTrue(!parseJson(R"(nul)").ok, "json: truncated literal is rejected");
        testResult.expectTrue(!parseJson(R"({"a": 1} garbage)").ok, "json: trailing content after value is rejected");
    }

    void testJsonParserHandlesNestedStructures(snodec::control::test::TestResult& testResult) {
        const auto result = parseJson(R"({"relations": {"needs": ["a", "b"], "excludes": []}, "value": {"nested": {"deep": [1, 2, 3]}}})");

        testResult.expectTrue(result.ok, "json: nested objects/arrays parse successfully");
        const JsonValue* needs = result.value.find("relations")->find("needs");
        testResult.expectEqual(2, static_cast<int>(needs->asStringArray().size()), "json: nested array inside object decodes correctly");
        const JsonValue* excludes = result.value.find("relations")->find("excludes");
        testResult.expectTrue(excludes->isArray() && excludes->items().empty(), "json: empty nested array decodes correctly");
        const JsonValue* deep = result.value.find("value")->find("nested")->find("deep");
        testResult.expectEqual(3, static_cast<int>(deep->items().size()), "json: doubly-nested array decodes correctly");
    }

    // Round-trip test (Step 2's testing requirement): parses every "#@" block in a real captured
    // `-s` output and asserts on the *decoded structure*, not on substring matches against the raw
    // text.
    void testParseMetaBlocksOnRealCapturedOutput(snodec::control::test::TestResult& testResult) {
        const ParsedMetadata metadata = parseMetaBlocks(snodec::control::test::fixtures::nestedSubCommandShowConfigOutput);

        testResult.expectTrue(metadata.present, "metadata: present is true for a target that emits #@ blocks");
        testResult.expectTrue(metadata.schemaRecognized, "metadata: schema/version are recognized");
        testResult.expectTrue(metadata.usable(), "metadata: usable() is true end-to-end");
        testResult.expectTrue(metadata.warnings.empty(), "metadata: no warnings for well-formed real output");

        testResult.expectTrue(metadata.document.has_value(), "metadata: document block decoded");
        if (metadata.document.has_value()) {
            testResult.expectEqual(std::string("snodec.config.comment-meta"), metadata.document->schema, "metadata: document schema decoded correctly");
            testResult.expectEqual(1, metadata.document->version, "metadata: document version decoded correctly");
            testResult.expectEqual(std::string("configurable-options-only"), metadata.document->scope, "metadata: document scope decoded correctly");
        }

        const MetaNode* root = findNode(metadata, {});
        testResult.expectTrue(root != nullptr, "metadata: root node (path []) decoded");
        if (root != nullptr) {
            testResult.expectEqual(std::string("application"), root->kind, "metadata: root node kind is application");
            testResult.expectEqual(std::string("root"), root->kindSource, "metadata: root node kindSource is root");
        }

        const MetaNode* topLevelTool = findNode(metadata, {"tool"});
        testResult.expectTrue(topLevelTool != nullptr, "metadata: top-level custom subcommand node decoded");
        if (topLevelTool != nullptr) {
            testResult.expectEqual(std::string("category"), topLevelTool->kind, "metadata: top-level custom subcommand kind is category");
        }

        const MetaNode* instanceNode = findNode(metadata, {"echoserver"});
        testResult.expectTrue(instanceNode != nullptr, "metadata: Instances-group node decoded");
        if (instanceNode != nullptr) {
            testResult.expectEqual(std::string("instance"), instanceNode->kind, "metadata: Instances-group node kind is instance");
            testResult.expectTrue(!instanceNode->disabled, "metadata: echoserver node is not disabled");
        }

        const MetaNode* outerNode = findNode(metadata, {"echoserver", "outer"});
        testResult.expectTrue(outerNode != nullptr, "metadata: custom node nested inside an instance decoded");

        const MetaNode* innerNode = findNode(metadata, {"echoserver", "outer", "inner"});
        testResult.expectTrue(innerNode != nullptr, "metadata: custom node nested three levels deep decoded");
        if (innerNode != nullptr) {
            testResult.expectEqual(std::string("category"), innerNode->kind, "metadata: three-levels-deep custom node kind is category");
        }

        const MetaNode* legacyNode = findNode(metadata, {"legacy"});
        testResult.expectTrue(legacyNode != nullptr, "metadata: disabled node decoded");
        if (legacyNode != nullptr) {
            testResult.expectTrue(legacyNode->disabled, "metadata: disabled node's disabled field decodes to true");
        }

        const MetaOption* deepOption = findOption(metadata, "echoserver.outer.inner.depth-value");
        testResult.expectTrue(deepOption != nullptr, "metadata: deeply nested option decoded");
        if (deepOption != nullptr) {
            testResult.expectEqual(3, static_cast<int>(deepOption->nodePath.size()), "metadata: deeply nested option's nodePath has expected length");
            testResult.expectEqual(std::string("echoserver.outer.inner"),
                                   deepOption->nodePath.size() == 3
                                       ? deepOption->nodePath[0] + "." + deepOption->nodePath[1] + "." + deepOption->nodePath[2]
                                       : std::string("<wrong length>"),
                                   "metadata: deeply nested option's nodePath decodes correctly");
            testResult.expectEqual(std::string("Options (persistent)"), deepOption->group, "metadata: deeply nested option's group decodes correctly");
            testResult.expectTrue(deepOption->value.cppDefault.has_value() && *deepOption->value.cppDefault == "deep", "metadata: deeply nested option's cppDefault decodes correctly");
        }

        const MetaGroup* persistentGroup = nullptr;
        for (const MetaGroup& group : metadata.groups) {
            if (group.name == "Options (persistent)" && group.nodePath.empty()) {
                persistentGroup = &group;
                break;
            }
        }
        testResult.expectTrue(persistentGroup != nullptr, "metadata: root's persistent group decoded");
        if (persistentGroup != nullptr) {
            testResult.expectEqual(std::string("persistent"), persistentGroup->kind, "metadata: root's persistent group kind decoded correctly");
        }
    }

    void testUnrecognizedSchemaTriggersFallback(snodec::control::test::TestResult& testResult) {
        const std::string input = "#@ snodec.meta begin document\n"
                                  "#@ {\n"
                                  "#@   \"schema\": \"some.other.schema\",\n"
                                  "#@   \"version\": 1,\n"
                                  "#@   \"entity\": \"document\"\n"
                                  "#@ }\n"
                                  "#@ snodec.meta end document\n"
                                  "log-level=4\n";

        const ParsedMetadata metadata = parseMetaBlocks(input);

        testResult.expectTrue(metadata.present, "metadata: present is true even for an unrecognized schema (a metadata attempt was made)");
        testResult.expectTrue(!metadata.schemaRecognized, "metadata: unrecognized schema clears schemaRecognized");
        testResult.expectTrue(!metadata.usable(), "metadata: usable() is false for an unrecognized schema");
        testResult.expectTrue(!metadata.warnings.empty(), "metadata: unrecognized schema produces a visible warning");
    }

    void testMalformedOptionBlockIsDroppedNotFatal(snodec::control::test::TestResult& testResult) {
        const std::string input = "#@ snodec.meta begin document\n"
                                  "#@ {\n"
                                  "#@   \"schema\": \"snodec.config.comment-meta\",\n"
                                  "#@   \"version\": 1,\n"
                                  "#@   \"entity\": \"document\"\n"
                                  "#@ }\n"
                                  "#@ snodec.meta end document\n"
                                  "#@ snodec.meta begin node\n"
                                  "#@ {\n"
                                  "#@   \"schema\": \"snodec.config.comment-meta\",\n"
                                  "#@   \"version\": 1,\n"
                                  "#@   \"entity\": \"node\",\n"
                                  "#@   \"kind\": \"application\",\n"
                                  "#@   \"path\": []\n"
                                  "#@ }\n"
                                  "#@ snodec.meta end node\n"
                                  "#@ snodec.meta begin option\n"
                                  "#@ { this is not valid json\n"
                                  "#@ snodec.meta end option\n"
                                  "#@ snodec.meta begin option\n"
                                  "#@ {\n"
                                  "#@   \"schema\": \"snodec.config.comment-meta\",\n"
                                  "#@   \"version\": 1,\n"
                                  "#@   \"entity\": \"option\",\n"
                                  "#@   \"key\": \"log-level\"\n"
                                  "#@ }\n"
                                  "#@ snodec.meta end option\n"
                                  "log-level=4\n";

        const ParsedMetadata metadata = parseMetaBlocks(input);

        testResult.expectTrue(metadata.schemaRecognized, "metadata: one malformed option block does not clear schemaRecognized");
        testResult.expectTrue(metadata.usable(), "metadata: one malformed option block does not make the whole result unusable");
        testResult.expectEqual(1, static_cast<int>(metadata.options.size()), "metadata: the well-formed option after a malformed one is still decoded");
        testResult.expectTrue(!metadata.warnings.empty(), "metadata: the malformed option block produces a warning");
    }

} // namespace

int main() {
    snodec::control::test::TestResult testResult;

    testJsonParserRoundTripsScalarsAndEscapes(testResult);
    testJsonParserRejectsMalformedInput(testResult);
    testJsonParserHandlesNestedStructures(testResult);
    testParseMetaBlocksOnRealCapturedOutput(testResult);
    testUnrecognizedSchemaTriggersFallback(testResult);
    testMalformedOptionBlockIsDroppedNotFatal(testResult);

    return testResult.processResult();
}
