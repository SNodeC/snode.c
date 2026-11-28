/*
 * SNode.C - A Slim Toolkit for Network Communication
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

// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>

#include "mcp/Types.h"
#include "mcp/detail/Headers.h"
#include "tests/support/TestResult.h"

#include <initializer_list>
#include <map>
#include <nlohmann/json.hpp>
#include <optional>
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <string>
#endif
int main() {
    tests::support::TestResult test;
    using mcp::Json;
    const Json schema{
        {"type", "object"},
        {"properties", {{"nested", {{"type", "object"}, {"properties", {{"region", {{"type", "string"}, {"x-mcp-header", "Region"}}}}}}}}}};
    const auto parameters = mcp::detail::headerParameters(schema);
    test.expectTrue(parameters && parameters->size() == 1, "only statically reachable primitive annotations compile");
    if (parameters) {
        const auto headers = mcp::detail::parameterHeaders(*parameters, {{"nested", {{"region", "Hello, 世界"}}}});
        test.expectTrue(mcp::detail::decodeHeader(headers.at("Mcp-Param-Region")) == "Hello, 世界",
                        "nested exact path uses canonical UTF-8 base64 header encoding");
        test.expectTrue(mcp::detail::parameterHeaders(*parameters, Json::object()).empty(), "absent arguments omit headers");
    }
    for (const std::string value : {"ascii", " padded ", "", "line1\nline2", "=?base64?literal?="}) {
        test.expectTrue(mcp::detail::decodeHeader(mcp::detail::encodeHeader(value)) == value,
                        "header encoding round trips sentinel, whitespace and controls");
    }
    for (const Json invalid :
         {Json{{"type", "object"}, {"properties", {{"x", {{"type", "number"}, {"x-mcp-header", "X"}}}}}},
          Json{{"type", "object"}, {"properties", {{"x", {{"type", "string"}, {"x-mcp-header", ""}}}}}},
          Json{{"type", "object"},
               {"properties", {{"x", {{"type", "string"}, {"x-mcp-header", "X"}}}, {"y", {{"type", "string"}, {"x-mcp-header", "x"}}}}}},
          Json{{"type", "array"}, {"items", {{"type", "string"}, {"x-mcp-header", "X"}}}},
          Json{{"allOf", {schema}}}}) {
        test.expectTrue(!mcp::detail::headerParameters(invalid), "invalid type, name, collision or unreachable annotation is rejected");
    }
    test.expectTrue(!mcp::detail::decodeHeader("=?base64?bad!?=") && !mcp::detail::decodeHeader("bad\r\nvalue"),
                    "malformed or injected header values fail closed");
    return test.processResult();
}
