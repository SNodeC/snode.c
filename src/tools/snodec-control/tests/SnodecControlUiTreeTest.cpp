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
#include "ui/UiTree.h"

#include <string>

using snodec::control::ConfigModel;
using snodec::control::parseShowConfigOutput;
using snodec::control::ui::buildUiTree;
using snodec::control::ui::flattenVisibleNodes;
using snodec::control::ui::UiNode;
using snodec::control::ui::UiNodeType;

namespace {

    // Covers every derivation rule at once:
    //  - log-level: no dot at all -> Application Options (rule 1).
    //  - echoserver.enabled: exactly 2 components, "echoserver" confirmed as an instance elsewhere
    //    (via echoserver.local.port) -> a direct option under Instance echoserver (rule 3, confirmed).
    //  - echoserver.local.port / echoserver.local.host: 3 components -> Instance echoserver, Section
    //    "local" (rule 2). This is one of the exact examples from the spec.
    //  - mqttbridge.broker.connection.host: 4 components -> Instance mqttbridge, Section
    //    "broker.connection" taken as one flat name, not nested (rule 2). The other exact spec example.
    //  - standalonesection.timeout: exactly 2 components, but "standalonesection" is never confirmed as
    //    an instance anywhere else -> falls into the "Other Options" fallback (rule 3, not confirmed).
    ConfigModel sampleModel() {
        const std::string input = "# Log level\n"
                                  "#log-level=4\n"
                                  "log-level=4\n"
                                  "\n"
                                  "# Enable echoserver\n"
                                  "#echoserver.enabled=true\n"
                                  "echoserver.enabled=true\n"
                                  "\n"
                                  "# Local port\n"
                                  "#echoserver.local.port=\"<REQUIRED>\"\n"
                                  "\n"
                                  "# Local host\n"
                                  "#echoserver.local.host=\"0.0.0.0\"\n"
                                  "echoserver.local.host=\"0.0.0.0\"\n"
                                  "\n"
                                  "# Broker host\n"
                                  "#mqttbridge.broker.connection.host=localhost\n"
                                  "mqttbridge.broker.connection.host=localhost\n"
                                  "\n"
                                  "# Standalone timeout\n"
                                  "#standalonesection.timeout=30\n"
                                  "standalonesection.timeout=30\n";

        return parseShowConfigOutput(input).model;
    }

    const UiNode* findChild(const std::vector<UiNode>& nodes, const std::string& label) {
        for (const UiNode& node : nodes) {
            if (node.label == label) {
                return &node;
            }
        }
        return nullptr;
    }

    void testApplicationOptions(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        auto tree = buildUiTree(model);

        testResult.expectTrue(!tree.topLevel.empty(), "tree: has at least one top-level node");
        testResult.expectTrue(tree.topLevel[0].type == UiNodeType::ApplicationOptions, "tree: first top-level node is ApplicationOptions");
        testResult.expectEqual(std::string("Application Options"), tree.topLevel[0].label, "tree: ApplicationOptions label");
        testResult.expectEqual(1, static_cast<int>(tree.topLevel[0].children.size()), "tree: exactly one dot-less option (log-level)");
        if (!tree.topLevel[0].children.empty()) {
            testResult.expectTrue(tree.topLevel[0].children[0].type == UiNodeType::Option, "tree: Application Options child is an Option");
            testResult.expectEqual(std::string("log-level"), tree.topLevel[0].children[0].label, "tree: Application Options child label");
        }
    }

    void testInstancesRootAndOrder(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        auto tree = buildUiTree(model);

        testResult.expectTrue(tree.topLevel.size() >= 2, "tree: has an Instances root");
        const UiNode& instancesRoot = tree.topLevel[1];
        testResult.expectTrue(instancesRoot.type == UiNodeType::InstancesRoot, "tree: second top-level node is InstancesRoot");
        testResult.expectEqual(std::string("Instances"), instancesRoot.label, "tree: InstancesRoot label");
        testResult.expectEqual(2, static_cast<int>(instancesRoot.children.size()), "tree: two instances discovered (echoserver, mqttbridge)");
        if (instancesRoot.children.size() == 2) {
            testResult.expectEqual(std::string("echoserver"), instancesRoot.children[0].label, "tree: echoserver is the first instance (discovery order)");
            testResult.expectEqual(std::string("mqttbridge"), instancesRoot.children[1].label, "tree: mqttbridge is the second instance");
        }
    }

    void testEchoserverInstanceDirectOptionAndSection(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        auto tree = buildUiTree(model);

        const UiNode* instancesRoot = findChild(tree.topLevel, "Instances");
        testResult.expectTrue(instancesRoot != nullptr, "echoserver: Instances root exists");
        if (instancesRoot == nullptr) {
            return;
        }

        const UiNode* echoserver = findChild(instancesRoot->children, "echoserver");
        testResult.expectTrue(echoserver != nullptr, "echoserver: instance node exists");
        if (echoserver == nullptr) {
            return;
        }
        testResult.expectTrue(echoserver->type == UiNodeType::Instance, "echoserver: node type is Instance");

        // rule 3 (confirmed): echoserver.enabled is a direct Option child, not wrapped in a Section.
        const UiNode* enabled = findChild(echoserver->children, "enabled");
        testResult.expectTrue(enabled != nullptr, "echoserver: 'enabled' is a direct child (rule 3, confirmed instance)");
        if (enabled != nullptr) {
            testResult.expectTrue(enabled->type == UiNodeType::Option, "echoserver: 'enabled' is an Option node");
        }

        // rule 2: echoserver.local.{port,host} become Instance echoserver / Section local / Option {port,host}.
        const UiNode* sectionsRoot = findChild(echoserver->children, "Sections");
        testResult.expectTrue(sectionsRoot != nullptr, "echoserver: has a Sections root");
        if (sectionsRoot == nullptr) {
            return;
        }
        testResult.expectTrue(sectionsRoot->type == UiNodeType::SectionsRoot, "echoserver: Sections root has the right type");

        const UiNode* local = findChild(sectionsRoot->children, "local");
        testResult.expectTrue(local != nullptr, "echoserver: 'local' section exists (echoserver.local.port example)");
        if (local == nullptr) {
            return;
        }
        testResult.expectTrue(local->type == UiNodeType::Section, "echoserver.local: node type is Section");
        testResult.expectEqual(2, static_cast<int>(local->children.size()), "echoserver.local: has port and host options");

        const UiNode* port = findChild(local->children, "port");
        testResult.expectTrue(port != nullptr && port->option != nullptr, "echoserver.local.port: leaf resolves to a ConfigOption");
        if (port != nullptr && port->option != nullptr) {
            testResult.expectEqual(std::string("echoserver.local"), port->option->section, "echoserver.local.port: full key is preserved via the cached ConfigOption");
            testResult.expectEqual(std::string("port"), port->option->key, "echoserver.local.port: leaf key is preserved");
        }
    }

    void testMqttbridgeFlatSectionExample(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        auto tree = buildUiTree(model);

        const UiNode* instancesRoot = findChild(tree.topLevel, "Instances");
        testResult.expectTrue(instancesRoot != nullptr, "mqttbridge: Instances root exists");
        if (instancesRoot == nullptr) {
            return;
        }

        const UiNode* mqttbridge = findChild(instancesRoot->children, "mqttbridge");
        testResult.expectTrue(mqttbridge != nullptr, "mqttbridge: instance node exists");
        if (mqttbridge == nullptr) {
            return;
        }

        const UiNode* sectionsRoot = findChild(mqttbridge->children, "Sections");
        testResult.expectTrue(sectionsRoot != nullptr, "mqttbridge: has a Sections root");
        if (sectionsRoot == nullptr) {
            return;
        }

        // mqttbridge.broker.connection.host -> Section "broker.connection", kept flat (not nested into
        // "broker" -> "connection"), exactly as the spec's example requires.
        const UiNode* section = findChild(sectionsRoot->children, "broker.connection");
        testResult.expectTrue(section != nullptr, "mqttbridge: 'broker.connection' is one flat section, not nested");
        if (section == nullptr) {
            return;
        }

        const UiNode* host = findChild(section->children, "host");
        testResult.expectTrue(host != nullptr && host->option != nullptr, "mqttbridge.broker.connection.host: leaf resolves correctly");
        if (host != nullptr && host->option != nullptr) {
            testResult.expectEqual(
                std::string("mqttbridge.broker.connection"), host->option->section, "mqttbridge.broker.connection.host: section preserved");
        }
    }

    void testOtherOptionsFallback(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        auto tree = buildUiTree(model);

        const UiNode* other = findChild(tree.topLevel, "Other Options");
        testResult.expectTrue(other != nullptr, "other options: unconfirmed 'standalonesection' is never silently discarded");
        if (other == nullptr) {
            return;
        }
        testResult.expectTrue(other->type == UiNodeType::Section, "other options: fallback node reuses the Section type");

        const UiNode* timeout = findChild(other->children, "timeout");
        testResult.expectTrue(timeout != nullptr && timeout->option != nullptr, "other options: 'timeout' option is present directly");
    }

    void testFlattenRespectsExpansion(snodec::control::test::TestResult& testResult) {
        ConfigModel model = sampleModel();
        auto tree = buildUiTree(model);

        // Default expansion: ApplicationOptions and InstancesRoot are expanded, everything below an
        // Instance is collapsed, so instance children (and the "Other Options" fallback) stay hidden.
        auto rows = flattenVisibleNodes(tree);
        std::size_t optionRows = 0;
        std::size_t instanceRows = 0;
        for (const auto& row : rows) {
            if (row.node->type == UiNodeType::Option) {
                ++optionRows;
            }
            if (row.node->type == UiNodeType::Instance) {
                ++instanceRows;
            }
        }
        testResult.expectEqual(1, static_cast<int>(optionRows), "flatten (collapsed): only the Application Option is visible");
        testResult.expectEqual(2, static_cast<int>(instanceRows), "flatten (collapsed): both instance headers are visible");

        // Expanding the first instance (echoserver) should reveal its direct option and its Sections
        // header, but not yet the options nested inside "local" (still collapsed).
        for (UiNode& node : tree.topLevel[1].children) {
            if (node.label == "echoserver") {
                node.expanded = true;
            }
        }
        auto rowsAfterExpand = flattenVisibleNodes(tree);
        testResult.expectTrue(rowsAfterExpand.size() > rows.size(), "flatten (expanded): expanding a node reveals more rows");

        bool sawEnabledOption = false;
        bool sawPortOption = false;
        for (const auto& row : rowsAfterExpand) {
            if (row.node->type == UiNodeType::Option && row.node->label == "enabled") {
                sawEnabledOption = true;
            }
            if (row.node->type == UiNodeType::Option && row.node->label == "port") {
                sawPortOption = true;
            }
        }
        testResult.expectTrue(sawEnabledOption, "flatten (expanded): echoserver's direct 'enabled' option becomes visible");
        testResult.expectTrue(!sawPortOption, "flatten (expanded): 'local' section's options stay hidden until it is expanded too");
    }

} // namespace

int main() {
    snodec::control::test::TestResult testResult;

    testApplicationOptions(testResult);
    testInstancesRootAndOrder(testResult);
    testEchoserverInstanceDirectOptionAndSection(testResult);
    testMqttbridgeFlatSectionExample(testResult);
    testOtherOptionsFallback(testResult);
    testFlattenRespectsExpansion(testResult);

    return testResult.processResult();
}
