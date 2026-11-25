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
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#endif
int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    {
        using mcp::Json;
        namespace detail = mcp::server::detail;
        mcp::server::Server server("logs", "1");
        auto channel = std::make_shared<detail::Channel>();
        auto other = std::make_shared<detail::Channel>();
        std::vector<Json> messages;
        std::vector<Json> independent;
        channel->output = [&](const Json& value) {
            messages.push_back(value);
        };
        other->output = [&](const Json& value) {
            independent.push_back(value);
        };
        const auto call = [&](const std::string& method, Json params, const auto& target, bool modern = false) {
            Json result;
            if (modern) {
                params["_meta"] = {{mcp::versionKey, mcp::protocolVersion2026}, {mcp::capabilitiesKey, Json::object()}};
            }
            detail::Access::registry(server).dispatch(
                {{"jsonrpc", "2.0"}, {"id", 1}, {"method", method}, {"params", params}},
                modern ? mcp::ProtocolEra::PerRequest20260728 : mcp::ProtocolEra::Initialization20251125,
                [&](const Json& value, bool) {
                    result = value;
                },
                utils::Timeval(30),
                {},
                target);
            return result;
        };
        const Json initialize{{"protocolVersion", mcp::protocolVersion2025},
                              {"capabilities", Json::object()},
                              {"clientInfo", {{"name", "test"}, {"version", "1"}}}};
        test.expectTrue(call("initialize", initialize, channel)["result"]["capabilities"].contains("logging"), "legacy logging advertised");
        call("initialize", initialize, other);
        test.expectTrue(!call("server/discover", Json::object(), channel, true)["result"]["capabilities"].contains("logging"),
                        "modern logging not advertised");
        test.expectTrue(call("logging/setLevel", {{"level", "warning"}}, channel)["result"].empty(),
                        "legacy setLevel returns empty result");
        test.expectTrue(call("logging/setLevel", {{"level", "trace"}}, channel)["error"]["code"] == -32602, "invalid level rejected");
        test.expectTrue(call("logging/setLevel", {{"level", "debug"}}, channel, true)["error"]["code"] == -32601,
                        "modern logging method unavailable");
        server.log(mcp::LogLevel::Info, {{"count", 1}}, "test");
        test.expectTrue(messages.empty() && independent.size() == 1, "threshold is per session");
        server.log(mcp::LogLevel::Error, {{"reason", "example"}}, "test");
        test.expectTrue(messages.size() == 1 &&
                            messages.back() ==
                                Json({{"jsonrpc", "2.0"},
                                      {"method", "notifications/message"},
                                      {"params", {{"level", "error"}, {"logger", "test"}, {"data", {{"reason", "example"}}}}}}),
                        "structured legacy log wire shape");
        std::vector<Json> contextMessages;
        server.tool({"context", {}, {{"type", "object"}}}, [&](const auto& request, const auto& reply) {
            contextMessages.push_back(request.context.log(mcp::LogLevel::Warning, nullptr));
            reply->result(mcp::ToolResult::text("done"));
        });
        call("tools/call", {{"name", "context"}}, channel);
        call("tools/call", {{"name", "context"}}, channel, true);
        test.expectTrue(contextMessages == std::vector<Json>{true, false} && messages.size() == 2,
                        "request context logs only for its legacy era and retains explicit null data");
        test.expectTrue(messages.back()["params"]["data"].is_null() && !messages.back()["params"].contains("logger"),
                        "optional logger omitted");
        const auto before = messages.size();
        server.log(mcp::LogLevel::Error, std::string(16385, 'x'));
        server.log(mcp::LogLevel::Error, 1, std::string(129, 'x'));
        test.expectTrue(messages.size() == before, "oversized payload and logger suppressed");
        for (int i = 0; i < 200; ++i) {
            server.log(mcp::LogLevel::Emergency, i);
        }
        test.expectTrue(messages.size() == 128 && independent.size() == 128, "per-session logging bounded to 128 messages per second");
        channel->logReset = utils::Timeval(0);
        server.log(mcp::LogLevel::Emergency, "next-window");
        test.expectTrue(messages.size() == 129 && independent.size() == 128, "independent rate windows reset on admission");
        channel->active = false;
        server.log(mcp::LogLevel::Emergency, "closed");
        test.expectTrue(messages.size() == 129, "closed session receives no log");
    }
    core::SNodeC::free();
    return test.processResult();
}
