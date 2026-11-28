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
#include "mcp/Results.h"
#include "mcp/Types.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Server.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"
#include "utils/Timeval.h"

#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <map>
#include <memory>
#include <optional>
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#endif

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    for (const auto era : {mcp::ProtocolEra::Initialization20251125, mcp::ProtocolEra::PerRequest20260728}) {
        mcp::server::Server server("pages", "1");
        for (const std::string name : {"c", "b", "a"}) {
            server.tool({name, {}, {{"type", "object"}}}, [](const auto&, const auto& reply) {
                reply->result(mcp::ToolResult::text("ok"));
            });
            server.resource({"test://" + name, name}, [](const auto&, const auto& reply) {
                reply->result(mcp::ResourceResult{});
            });
            server.resourceTemplate({"test://" + name + "/{id}", name}, [](const auto&, const auto& reply) {
                reply->result(mcp::ResourceResult{});
            });
            server.prompt({name, {}, {}}, [](const auto&, const auto& reply) {
                reply->result(mcp::PromptResult{});
            });
        }
        const auto call = [&](const std::string& method, mcp::Json params, std::size_t size = 2) {
            mcp::Json output;
            if (era == mcp::ProtocolEra::PerRequest20260728) {
                params["_meta"] = {{mcp::versionKey, mcp::protocolVersion2026}, {mcp::capabilitiesKey, mcp::Json::object()}};
            }
            mcp::server::detail::Access::registry(server).dispatch(
                {{"jsonrpc", "2.0"}, {"id", 1}, {"method", method}, {"params", params}},
                era,
                [&](const mcp::Json& value, bool) {
                    output = value;
                },
                utils::Timeval(30),
                {},
                {},
                size);
            return output;
        };
        for (const std::string method : {"tools/list", "resources/list", "resources/templates/list", "prompts/list"}) {
            const std::string field = method == "resources/templates/list" ? "resourceTemplates" : method.substr(0, method.find('/'));
            const auto first = call(method, mcp::Json::object());
            test.expectTrue(first["result"][field].size() == 2 && first["result"][field][0]["name"] == "a" &&
                                first["result"].contains("nextCursor"),
                            "first page has stable lexical ordering and an opaque continuation");
            const auto cursor = first["result"]["nextCursor"];
            const auto last = call(method, {{"cursor", cursor}});
            test.expectTrue(last["result"][field].size() == 1 && last["result"][field][0]["name"] == "c" &&
                                !last["result"].contains("nextCursor"),
                            "last page omits nextCursor");
            if (era == mcp::ProtocolEra::PerRequest20260728) {
                test.expectTrue(first["result"]["ttlMs"] == 0 && last["result"]["ttlMs"] == 0 &&
                                    first["result"]["cacheScope"] == "private" && last["result"]["cacheScope"] == "private",
                                "each modern page has consistent cache scope and its own freshness hint");
            }
            test.expectTrue(call(method, {{"cursor", cursor}}, 1)["error"]["code"] == -32602, "cursor binds the page size");
            const std::string other = method == "tools/list" ? "resources/list" : "tools/list";
            test.expectTrue(call(other, {{"cursor", cursor}})["error"]["code"] == -32602, "cursor cannot cross list methods");
            for (const auto& invalid : std::vector<mcp::Json>{"", "not a cursor", nullptr, 1, true}) {
                test.expectTrue(call(method, {{"cursor", invalid}})["error"]["code"] == -32602, "malformed cursor returns Invalid params");
            }
        }
        const auto first = call("tools/list", mcp::Json::object());
        server.remove(mcp::server::RegistrationKind::Tool, "b");
        test.expectTrue(call("tools/list", {{"cursor", first["result"]["nextCursor"]}})["error"]["code"] == -32602,
                        "registry mutation invalidates outstanding cursors");
        const auto restarted = call("tools/list", mcp::Json::object());
        test.expectTrue(restarted["result"]["tools"].size() == 2 && !restarted["result"].contains("nextCursor"),
                        "restart sees the current complete registry");
    }
    core::SNodeC::free();
    return test.processResult();
}
