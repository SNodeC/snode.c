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

#include "core/SNodeC.h"
#include "mcp/Protocol.h"
#include "mcp/Types.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Results.h"
#include "mcp/server/Server.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"

#include <cmath>
#include <map>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <initializer_list>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace mcp::server;

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    Implementation implementation{"metadata", "1", "Display", "Description", "https://example.com", {{"https://example.com/icon.png"}}};
    Server server(implementation);
    ResourceDefinition definition{"test://meta", "metadata"};
    definition.icons = implementation.icons;
    definition.annotations.priority = 0.5;
    definition.size = 7;
    definition.meta = {{"com.example/key", 7}};
    server.resource(definition, [](const auto& request, const auto& reply) {
        ResourceResult result{{TextResource{request.uri, "text/plain", "content", {{"com.example/key", 8}}}}};
        result.cache = CacheHints{1000, CacheScope::Public};
        result.meta = {{"com.example/key", 9}};
        reply->result(std::move(result));
    });
    for (const auto era : {detail::ProtocolEra::Initialization20251125, detail::ProtocolEra::PerRequest20260728}) {
        const bool modern = era == detail::ProtocolEra::PerRequest20260728;
        const auto dispatch = [&](const std::string& method, Json params) {
            Json result;
            if (modern) {
                params["_meta"] = {{detail::versionKey, detail::protocolVersion2026}, {detail::capabilitiesKey, Json::object()}};
            }

            detail::Access::registry(server).dispatch(
                {{"jsonrpc", "2.0"}, {"id", 1}, {"method", method}, {"params", params}}, era, [&](const Json& value, bool) {
                    result = value;
                });

            return result["result"];
        };
        const auto listed = dispatch("resources/list", Json::object())["resources"][0];
        test.expectTrue(listed["icons"][0]["src"] == implementation.icons[0].src && listed["annotations"]["priority"] == 0.5 &&
                            listed["size"] == 7 && listed["_meta"]["com.example/key"] == 7,
                        "definition metadata preserved in both eras");
        const auto read = dispatch("resources/read", {{"uri", definition.uri}});
        test.expectTrue(read["contents"][0]["_meta"]["com.example/key"] == 8 && read["_meta"]["com.example/key"] == 9,
                        "content/result metadata preserved");
        test.expectTrue(modern ? read["cacheScope"] == "public" && read["ttlMs"] == 1000
                               : !read.contains("cacheScope") && !read.contains("resultType"),
                        "cache policy and result kinds respect protocol era");
    }

    core::SNodeC::free();

    return test.processResult();
}
