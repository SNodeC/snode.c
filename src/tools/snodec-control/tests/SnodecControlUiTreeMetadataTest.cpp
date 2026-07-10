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
#include "ui/UiTree.h"

#include <string>

using snodec::control::ConfigModel;
using snodec::control::parseMetaBlocks;
using snodec::control::parseShowConfigOutput;
using snodec::control::ParsedMetadata;
using snodec::control::ui::buildUiTree;
using snodec::control::ui::flattenVisibleNodes;
using snodec::control::ui::UiNode;
using snodec::control::ui::UiNodeType;
using snodec::control::ui::UiTree;

namespace {

    const UiNode* findChild(const std::vector<UiNode>& nodes, const std::string& label) {
        for (const UiNode& node : nodes) {
            if (node.label == label) {
                return &node;
            }
        }
        return nullptr;
    }

    struct Fixture {
        ConfigModel model;
        ParsedMetadata metadata;
        UiTree tree;
    };

    Fixture buildFromRealCapture() {
        Fixture fixture;
        fixture.model = parseShowConfigOutput(snodec::control::test::fixtures::nestedSubCommandShowConfigOutput).model;
        fixture.metadata = parseMetaBlocks(snodec::control::test::fixtures::nestedSubCommandShowConfigOutput);
        fixture.tree = buildUiTree(fixture.model, fixture.metadata);
        return fixture;
    }

    void testMetadataTreeHasSingleApplicationRoot(snodec::control::test::TestResult& testResult) {
        Fixture fixture = buildFromRealCapture();

        testResult.expectEqual(1, static_cast<int>(fixture.tree.topLevel.size()), "metadata tree: exactly one top-level node (the application root) when every option matches metadata");
        if (!fixture.tree.topLevel.empty()) {
            testResult.expectTrue(fixture.tree.topLevel[0].type == UiNodeType::Node, "metadata tree: root is a Node");
            testResult.expectEqual(std::string("application"), fixture.tree.topLevel[0].kind, "metadata tree: root kind is application");
            testResult.expectTrue(fixture.tree.topLevel[0].expanded, "metadata tree: root is expanded by default");
        }
    }

    void testTopLevelCustomCategoryAlongsideInstance(snodec::control::test::TestResult& testResult) {
        Fixture fixture = buildFromRealCapture();
        testResult.expectTrue(!fixture.tree.topLevel.empty(), "top-level tool: tree has a root");
        if (fixture.tree.topLevel.empty()) {
            return;
        }

        const UiNode* tool = findChild(fixture.tree.topLevel[0].children, "tool");
        testResult.expectTrue(tool != nullptr, "top-level tool: 'tool' node exists as a direct child of the root");
        if (tool != nullptr) {
            testResult.expectTrue(tool->type == UiNodeType::Node, "top-level tool: node type is Node");
            testResult.expectEqual(std::string("category"), tool->kind, "top-level tool: kind is category (named subcommand, non-Instances/Sections group)");
        }
    }

    void testInstanceNodeAndNestedCustomSubcommands(snodec::control::test::TestResult& testResult) {
        Fixture fixture = buildFromRealCapture();
        testResult.expectTrue(!fixture.tree.topLevel.empty(), "nested subcommands: tree has a root");
        if (fixture.tree.topLevel.empty()) {
            return;
        }

        const UiNode* echoserver = findChild(fixture.tree.topLevel[0].children, "echoserver");
        testResult.expectTrue(echoserver != nullptr, "nested subcommands: 'echoserver' instance node exists");
        if (echoserver == nullptr) {
            return;
        }
        testResult.expectEqual(std::string("instance"), echoserver->kind, "nested subcommands: echoserver kind is instance");

        const UiNode* outer = findChild(echoserver->children, "outer");
        testResult.expectTrue(outer != nullptr, "nested subcommands: 'outer' is nested one level inside the instance");
        if (outer == nullptr) {
            return;
        }
        testResult.expectEqual(std::string("category"), outer->kind, "nested subcommands: outer kind is category");

        const UiNode* inner = findChild(outer->children, "inner");
        testResult.expectTrue(inner != nullptr, "nested subcommands: 'inner' is nested two levels inside the instance (three below root)");
        if (inner == nullptr) {
            return;
        }
        testResult.expectEqual(std::string("category"), inner->kind, "nested subcommands: inner kind is category");

        // depth-value, like every option addOption() adds without an explicit group, lands in the
        // "Options (persistent)" group (see utils::SubCommand::setConfigurable()) - an implicit group
        // that is flattened away (every option shown here is already persistent/configurable, so that
        // layer is redundant; see isImplicitOptionsGroup() in UiTree.cpp), so depth-value is a direct
        // child of "inner", not nested one level further under a group node.
        testResult.expectTrue(findChild(inner->children, "Options (persistent)") == nullptr,
                              "nested subcommands: 'inner' has no redundant 'Options (persistent)' group node");
        const UiNode* depthOption = findChild(inner->children, "depth-value");
        testResult.expectTrue(depthOption != nullptr && depthOption->option != nullptr,
                              "nested subcommands: the deeply nested option is a direct child of 'inner' and resolves to a live ConfigOption");
    }

    void testDisabledNodeIsSurfaced(snodec::control::test::TestResult& testResult) {
        Fixture fixture = buildFromRealCapture();
        testResult.expectTrue(!fixture.tree.topLevel.empty(), "disabled node: tree has a root");
        if (fixture.tree.topLevel.empty()) {
            return;
        }

        const UiNode* legacy = findChild(fixture.tree.topLevel[0].children, "legacy");
        testResult.expectTrue(legacy != nullptr, "disabled node: 'legacy' node exists");
        if (legacy != nullptr) {
            testResult.expectTrue(legacy->disabled, "disabled node: disabled field is surfaced as true");
        }
    }

    // Every option shown in the config UI is already persistent/configurable (Formatter.cpp never
    // emits a non-configurable option into "-s" output at all), so the implicit "Options
    // (persistent)"/"default" group is redundant as a visible tree level and must be flattened: its
    // options attach directly to the owning node instead of nesting one level deeper under a group
    // node.
    void testImplicitPersistentGroupIsFlattenedNotVisible(snodec::control::test::TestResult& testResult) {
        Fixture fixture = buildFromRealCapture();
        testResult.expectTrue(!fixture.tree.topLevel.empty(), "group flattening: tree has a root");
        if (fixture.tree.topLevel.empty()) {
            return;
        }

        const UiNode* persistentGroup = findChild(fixture.tree.topLevel[0].children, "Options (persistent)");
        testResult.expectTrue(persistentGroup == nullptr, "group flattening: root has no visible 'Options (persistent)' group node");

        const UiNode* logLevel = findChild(fixture.tree.topLevel[0].children, "log-level");
        testResult.expectTrue(logLevel != nullptr && logLevel->option != nullptr,
                              "group flattening: log-level is instead a direct child of the root and still resolves to a live ConfigOption");
    }

    void testNoUnmatchedOptionsForWellFormedRealCapture(snodec::control::test::TestResult& testResult) {
        Fixture fixture = buildFromRealCapture();

        const UiNode* unmatched = findChild(fixture.tree.topLevel, "Unmatched Options");
        testResult.expectTrue(unmatched == nullptr, "unmatched fallback: every option in a well-formed real capture is matched, so the fallback bucket never appears");
    }

    void testFlattenVisitsMetadataTreeConsistently(snodec::control::test::TestResult& testResult) {
        Fixture fixture = buildFromRealCapture();

        // Expand everything reachable so flatten's depth-first walk is exercised end-to-end.
        std::vector<UiNode*> stack;
        for (UiNode& node : fixture.tree.topLevel) {
            stack.push_back(&node);
        }
        while (!stack.empty()) {
            UiNode* node = stack.back();
            stack.pop_back();
            if (node->type != UiNodeType::Option) {
                node->expanded = true;
                for (UiNode& child : node->children) {
                    stack.push_back(&child);
                }
            }
        }

        const auto rows = flattenVisibleNodes(fixture.tree);
        bool sawDeepOption = false;
        for (const auto& row : rows) {
            if (row.node->type == UiNodeType::Option && row.node->label == "depth-value") {
                sawDeepOption = true;
                testResult.expectTrue(row.depth >= 4, "flatten: the three-levels-deep option is reported at a correspondingly large depth, not clamped");
            }
        }
        testResult.expectTrue(sawDeepOption, "flatten: fully expanding the metadata tree reaches the deeply nested option");
    }

    void testEmptyMetadataFallsBackToLegacyHeuristic(snodec::control::test::TestResult& testResult) {
        const std::string input = "# Log level\n"
                                  "#log-level=4\n"
                                  "log-level=4\n";
        ConfigModel model = parseShowConfigOutput(input).model;
        const ParsedMetadata emptyMetadata; // present == false

        const UiTree metadataTree = buildUiTree(model, emptyMetadata);
        const UiTree legacyTree = buildUiTree(model);

        testResult.expectEqual(static_cast<int>(legacyTree.topLevel.size()),
                               static_cast<int>(metadataTree.topLevel.size()),
                               "fallback: absent metadata produces exactly the same top-level shape as the legacy builder");
        if (!metadataTree.topLevel.empty()) {
            testResult.expectTrue(metadataTree.topLevel[0].type == UiNodeType::ApplicationOptions,
                                  "fallback: absent metadata falls all the way back to the legacy ApplicationOptions node type");
        }
    }

} // namespace

int main() {
    snodec::control::test::TestResult testResult;

    testMetadataTreeHasSingleApplicationRoot(testResult);
    testTopLevelCustomCategoryAlongsideInstance(testResult);
    testInstanceNodeAndNestedCustomSubcommands(testResult);
    testDisabledNodeIsSurfaced(testResult);
    testImplicitPersistentGroupIsFlattenedNotVisible(testResult);
    testNoUnmatchedOptionsForWellFormedRealCapture(testResult);
    testFlattenVisitsMetadataTreeConsistently(testResult);
    testEmptyMetadataFallsBackToLegacyHeuristic(testResult);

    return testResult.processResult();
}
