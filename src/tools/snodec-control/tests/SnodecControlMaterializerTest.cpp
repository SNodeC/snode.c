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
#include "Materializer.h"
#include "TestResult.h"

#include <string>

using snodec::control::applyEdits;
using snodec::control::EditOperation;
using snodec::control::materialize;
using snodec::control::parseShowConfigOutput;

namespace {

    void testMaterializeFromDiscoveredModel(snodec::control::test::TestResult& testResult) {
        const std::string input = "# Log level\n"
                                  "log-level=4\n"
                                  "\n"
                                  "# Port number\n"
                                  "#server.local.port=\"<REQUIRED>\"\n"
                                  "server.local.port=9000\n"
                                  "\n"
                                  "# An empty default\n"
                                  "#server.local.tag=\"\"\n";

        const auto parseResult = parseShowConfigOutput(input);
        const std::string materialized = materialize(parseResult.model, "/path/to/target");

        testResult.expectTrue(materialized.find("[server.local]") != std::string::npos,
                              "materialize: dotted-prefix section is rendered as an INI section header");
        testResult.expectTrue(materialized.find("log-level=4") != std::string::npos,
                              "materialize: a global (unsectioned) option is written before any section header");
        testResult.expectTrue(materialized.find("port=9000") != std::string::npos,
                              "materialize: the active value is preferred over the default");
        testResult.expectTrue(materialized.find("# REQUIRED: this option must be configured") != std::string::npos,
                              "materialize: required options are marked with a comment");
        testResult.expectTrue(materialized.find("# Port number") != std::string::npos, "materialize: descriptions are emitted as comments");
        testResult.expectTrue(materialized.find("tag=\"\"") != std::string::npos,
                              "materialize: an empty default value is written as an explicit empty string");
    }

    void testMaterializeAfterEdits(snodec::control::test::TestResult& testResult) {
        const std::string input = "# Log level\n"
                                  "#log-level=4\n"
                                  "\n"
                                  "# Port number\n"
                                  "#server.local.port=\"<REQUIRED>\"\n"
                                  "\n"
                                  "# Host\n"
                                  "#server.local.host=\"0.0.0.0\"\n";

        auto parseResult = parseShowConfigOutput(input);

        applyEdits(parseResult.model,
                   {EditOperation{EditOperation::Kind::Set, "log-level", "5"},
                    EditOperation{EditOperation::Kind::Set, "server.local.port", "8080"}},
                   false);

        const std::string materialized = materialize(parseResult.model, "/path/to/target");

        testResult.expectTrue(materialized.find("log-level=5") != std::string::npos,
                              "materialize after edits: the --set active value is used");
        testResult.expectTrue(materialized.find("port=8080") != std::string::npos,
                              "materialize after edits: the --set active value overrides <REQUIRED>");
        testResult.expectTrue(materialized.find("# REQUIRED: this option must be configured") != std::string::npos,
                              "materialize after edits: the required marker is persistent metadata and stays even once a value is set");
        testResult.expectTrue(materialized.find("host=0.0.0.0") != std::string::npos,
                              "materialize after edits: an untouched option keeps its default");

        // Now unset the port again: materialize should fall back to an empty value, since the option has
        // no usable default (it was a <REQUIRED> placeholder).
        applyEdits(parseResult.model, {EditOperation{EditOperation::Kind::Unset, "server.local.port", ""}}, false);
        const std::string materializedAfterUnset = materialize(parseResult.model, "/path/to/target");

        testResult.expectTrue(materializedAfterUnset.find("port=\"\"") != std::string::npos,
                              "materialize after unset: falls back to an empty value when no default exists");
        testResult.expectTrue(materializedAfterUnset.find("# REQUIRED: this option must be configured") != std::string::npos,
                              "materialize after unset: the option is marked required again");
    }

} // namespace

int main() {
    snodec::control::test::TestResult testResult;

    testMaterializeFromDiscoveredModel(testResult);
    testMaterializeAfterEdits(testResult);

    return testResult.processResult();
}
