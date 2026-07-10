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
#include "JsonWriter.h"
#include "TestResult.h"

#include <string>

using snodec::control::jsonEscape;
using snodec::control::parseShowConfigOutput;
using snodec::control::toJson;

int main() {
    snodec::control::test::TestResult testResult;

    testResult.expectTrue(jsonEscape("plain") == "plain", "jsonEscape: plain text is unchanged");
    testResult.expectTrue(jsonEscape("a\"b\\c") == "a\\\"b\\\\c", "jsonEscape: quotes and backslashes are escaped");
    testResult.expectTrue(jsonEscape("line1\nline2") == "line1\\nline2", "jsonEscape: newlines are escaped");

    const std::string input = "# Port number\n"
                              "#server.local.port=\"<REQUIRED>\"\n"
                              "server.local.port=9000\n";

    const auto parseResult = parseShowConfigOutput(input);
    const std::string json = toJson(parseResult.model, "/path/to/target");

    testResult.expectTrue(json.find("\"target\": \"/path/to/target\"") != std::string::npos,
                          "toJson: target path is embedded in the document");
    testResult.expectTrue(json.find("\"name\": \"server.local\"") != std::string::npos, "toJson: section name is rendered");
    testResult.expectTrue(json.find("\"key\": \"port\"") != std::string::npos, "toJson: option key is rendered");
    testResult.expectTrue(json.find("\"activeValue\": \"9000\"") != std::string::npos, "toJson: active value is rendered");
    testResult.expectTrue(json.find("\"required\": true") != std::string::npos, "toJson: required flag is rendered as a JSON boolean");

    return testResult.processResult();
}
