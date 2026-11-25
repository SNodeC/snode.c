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

#include "core/SNodeC.h"
#include "mcp/Protocol.h"
#include "mcp/Types.h"
#include "mcp/server/Server.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <cstddef>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>
#endif
int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    using mcp::Json;
    namespace detail = mcp::server::detail;
    for (const bool modern : {false, true}) {
        mcp::server::Server server("limits", "1");
        mcp::server::ToolDefinition tool{"job", {}, {{"type", "object"}}};
        tool.taskSupport = mcp::TaskSupport::Optional;
        server.tool(tool, [](const auto& call, const auto& reply) {
            reply->startTask(
                mcp::TaskOptions{call.arguments.value("ttl", std::uint64_t(5000)), call.arguments.value("poll", std::uint64_t(10))});
        });
        auto channel = std::make_shared<detail::Channel>();
        const auto other = std::make_shared<detail::Channel>();
        const auto call = [&](Json task, bool second = false, Json arguments = Json::object()) {
            Json result;
            Json params{{"name", "job"}, {"arguments", arguments}};
            if (modern) {
                params["_meta"] = {{mcp::versionKey, mcp::protocolVersion2026},
                                   {mcp::capabilitiesKey, {{"extensions", {{"io.modelcontextprotocol/tasks", Json::object()}}}}}};
            } else {
                params["task"] = task;
            }
            const auto response = detail::Access::registry(server).dispatch(
                {{"jsonrpc", "2.0"}, {"id", 1}, {"method", "tools/call"}, {"params", params}},
                modern ? mcp::ProtocolEra::PerRequest20260728 : mcp::ProtocolEra::Initialization20251125,
                [&](const Json& value, bool) {
                    result = value;
                },
                utils::Timeval(30),
                mcp::Principal{second ? "second" : "first", {}},
                second ? other : channel);
            return result;
        };
        for (unsigned index = 0; index < 16; ++index) {
            test.expectTrue(call(Json::object()).contains("result"), "default sixteen tasks admitted per requestor");
        }
        test.expectTrue(call(Json::object())["error"]["code"] == -32000, "one requestor cannot exhaust the global store");
        test.expectTrue(call(Json::object(), true).contains("result"), "another requestor has an independent allowance");
        detail::Access::registry(server).endChannel(channel);
        detail::Access::registry(server).endChannel(other);
        channel = std::make_shared<detail::Channel>();
        server.setTasksPerRequestor(2)->setMaximumTaskTtlMs(1000);
        const auto clamped = call({{"ttl", 5000}})["result"];
        test.expectTrue((modern ? clamped : clamped["task"])[modern ? "ttlMs" : "ttl"] == 1000,
                        "reported TTL is the actual configured retention maximum");
        test.expectTrue(call(Json::object()).contains("result"), "configured second task admitted");
        test.expectTrue(call(Json::object())["error"]["code"] == -32000, "configured per-requestor allowance enforced");
        detail::Access::registry(server).endChannel(channel);
        channel = std::make_shared<detail::Channel>();
        if (modern) {
            test.expectTrue(call(Json::object(), false, {{"ttl", 0}})["error"]["code"] == -32602, "zero modern TTL is invalid params");
            test.expectTrue(call(Json::object(), false, {{"poll", 0}})["error"]["code"] == -32602, "zero poll interval is invalid params");
        } else {
            for (const auto& invalid :
                 std::vector<Json>{nullptr, 1, Json::array(), {{"ttl", 0}}, {{"ttl", -1}}, {{"ttl", 1.5}}, {{"ttl", "5"}}}) {
                test.expectTrue(call(invalid)["error"]["code"] == -32602, "malformed task augmentation is invalid params");
            }
        }
    }
    core::SNodeC::free();
    return test.processResult();
}
