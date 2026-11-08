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

// Regression coverage for two focused snodec-control fixes:
//
//  1. The implicit "Options (persistent)"/"default" metadata group must never appear as its own
//     visible UiTree node - every option shown in the config UI is already persistent/configurable,
//     so that layer is redundant (see isImplicitOptionsGroup() in ui/UiTree.cpp).
//  2. A description line shaped like "key = value" (e.g. "sni = SNI of the virtual server" inside
//     --sni-cert's own multi-line description) must never be parsed into a standalone phantom config
//     option. When structured "#@" metadata is present and usable, it is authoritative for option
//     identity: a legacy-parsed option backed only by a commented-default-looking line, with no
//     matching metadata entry, is discarded (applyMetadataToModel() in ConfigModel.cpp) - while a
//     real active/uncommented config line with no metadata match is still preserved as unmatched.

#include "ConfigModel.h"
#include "ConfigParser.h"
#include "Metadata.h"
#include "TestResult.h"
#include "ui/UiTree.h"

#include <algorithm>
#include <string>

using snodec::control::applyMetadataToModel;
using snodec::control::ConfigModel;
using snodec::control::ConfigOption;
using snodec::control::ConfigSection;
using snodec::control::ParsedMetadata;
using snodec::control::parseMetaBlocks;
using snodec::control::parseShowConfigOutput;
using snodec::control::ui::buildUiTree;
using snodec::control::ui::UiNode;
using snodec::control::ui::UiNodeType;
using snodec::control::ui::UiTree;

namespace {

    const ConfigOption* findOption(const ConfigModel& model, const std::string& key) {
        for (const ConfigSection& section : model.sections) {
            const auto it = std::find_if(section.options.begin(), section.options.end(), [&key](const ConfigOption& option) {
                return option.key == key;
            });
            if (it != section.options.end()) {
                return &(*it);
            }
        }
        return nullptr;
    }

    const UiNode* findChild(const std::vector<UiNode>& nodes, const std::string& label) {
        for (const UiNode& node : nodes) {
            if (node.label == label) {
                return &node;
            }
        }
        return nullptr;
    }

    // A minimal but realistic "-s"-shaped capture: a "sni-cert" option whose real, multi-line
    // description contains several "key = value"-shaped explanatory lines (mirroring
    // ConfigTlsServer's real wording, reproduced here only as literal test input text, never modified
    // in SNode.C itself), full "#@" metadata for exactly one option (sni-cert) and one implicit
    // persistent group, and one genuinely unknown active config line.
    const std::string capturedOutput = R"SNICAPTURE(#@ snodec.meta begin document
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1
#@ }
#@ snodec.meta end document
#@ snodec.meta begin node
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "kind": "application",
#@   "kindSource": "root",
#@   "displayName": "httpserver-tls-in",
#@   "path": [],
#@   "disabled": false
#@ }
#@ snodec.meta end node
#@ snodec.meta begin group
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "name": "Options (persistent)",
#@   "kind": "persistent",
#@   "nodePath": []
#@ }
#@ snodec.meta end group
#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "sni-cert",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "SNI certificate mapping. Example:\nsni = SNI of the virtual server\ncert = path to the certificate file\nkey = path to the private key file",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "list"
#@   },
#@   "relations": {
#@     "needs": [],
#@     "excludes": []
#@   }
#@ }
#@ snodec.meta end option

## Options (persistent)
# SNI certificate mapping. Example:
# sni = SNI of the virtual server
# cert = path to the certificate file
# key = path to the private key file
#sni-cert=""

unknown-active=some-value
)SNICAPTURE";

    void testSniDescriptionLineDoesNotCreatePhantomOption(snodec::control::test::TestResult& testResult) {
        ConfigModel model = parseShowConfigOutput(capturedOutput).model;
        const ParsedMetadata metadata = parseMetaBlocks(capturedOutput);
        testResult.expectTrue(metadata.usable(), "sni regression: metadata is usable for this capture");

        applyMetadataToModel(model, metadata);

        testResult.expectTrue(findOption(model, "sni") == nullptr, "sni regression: no phantom 'sni' option exists after reconciliation");
        testResult.expectTrue(findOption(model, "cert") == nullptr, "sni regression: no phantom 'cert' option exists after reconciliation");
        testResult.expectTrue(findOption(model, "key") == nullptr, "sni regression: no phantom 'key' option exists after reconciliation");

        const ConfigOption* sniCert = findOption(model, "sni-cert");
        testResult.expectTrue(sniCert != nullptr, "sni regression: the real 'sni-cert' option still exists");
        if (sniCert != nullptr) {
            testResult.expectTrue(
                sniCert->description.find("sni = SNI of the virtual server") != std::string::npos,
                "sni regression: sni-cert's own description still legitimately contains the 'sni = ...' explanatory line");
        }

        const UiTree tree = buildUiTree(model, metadata);
        testResult.expectTrue(!tree.topLevel.empty(), "sni regression: tree has a root");
        if (!tree.topLevel.empty()) {
            const UiNode* unmatched = findChild(tree.topLevel, "Unmatched Options");
            const bool sniInUnmatched = unmatched != nullptr && findChild(unmatched->children, "sni") != nullptr;
            testResult.expectTrue(!sniInUnmatched, "sni regression: 'sni' does not appear in Unmatched Options either");

            const UiNode* sniCertNode = findChild(tree.topLevel[0].children, "sni-cert");
            testResult.expectTrue(sniCertNode != nullptr && sniCertNode->option != nullptr,
                                  "sni regression: 'sni-cert' is visible as a direct child of the root (implicit group flattened)");
        }
    }

    void testRealUnmatchedActiveOptionIsPreserved(snodec::control::test::TestResult& testResult) {
        ConfigModel model = parseShowConfigOutput(capturedOutput).model;
        const ParsedMetadata metadata = parseMetaBlocks(capturedOutput);

        applyMetadataToModel(model, metadata);

        const ConfigOption* unknownActive = findOption(model, "unknown-active");
        testResult.expectTrue(unknownActive != nullptr,
                              "unmatched active: a real uncommented config line with no metadata match survives reconciliation");
        if (unknownActive != nullptr) {
            testResult.expectTrue(unknownActive->hasActiveValue && unknownActive->fromActiveLine,
                                  "unmatched active: it is correctly tracked as coming from an active line");
        }

        const UiTree tree = buildUiTree(model, metadata);
        const UiNode* unmatched = findChild(tree.topLevel, "Unmatched Options");
        testResult.expectTrue(unmatched != nullptr, "unmatched active: 'Unmatched Options' bucket exists");
        if (unmatched != nullptr) {
            testResult.expectTrue(findChild(unmatched->children, "unknown-active") != nullptr,
                                  "unmatched active: 'unknown-active' is visible under 'Unmatched Options'");
        }
    }

    void testNoRedundantPersistentGroupNodeForThisCapture(snodec::control::test::TestResult& testResult) {
        ConfigModel model = parseShowConfigOutput(capturedOutput).model;
        const ParsedMetadata metadata = parseMetaBlocks(capturedOutput);
        applyMetadataToModel(model, metadata);

        const UiTree tree = buildUiTree(model, metadata);
        testResult.expectTrue(!tree.topLevel.empty(), "group flattening: tree has a root");
        if (!tree.topLevel.empty()) {
            testResult.expectTrue(findChild(tree.topLevel[0].children, "Options (persistent)") == nullptr,
                                  "group flattening: no visible 'Options (persistent)' group node for this capture");
        }
    }

} // namespace

int main() {
    snodec::control::test::TestResult testResult;

    testSniDescriptionLineDoesNotCreatePhantomOption(testResult);
    testRealUnmatchedActiveOptionIsPreserved(testResult);
    testNoRedundantPersistentGroupNodeForThisCapture(testResult);

    return testResult.processResult();
}
