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
#include "core/timer/Timer.h"
#include "mcp/Protocol.h"
#include "mcp/Results.h"
#include "mcp/Types.h"
#include "mcp/server/Reply.h"
#include "mcp/server/Server.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"

#include <cmath>
#include <cstddef>
#include <functional>
#include <map>
#include <optional>
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
        namespace detail = mcp::server::detail;
        using mcp::Json;
        mcp::server::Server server("runtime", "1");
        auto& registry = detail::Access::registry(server);
        registry.attach();
        std::vector<Json> legacy;
        std::vector<Json> modern;
        std::vector<Json> ignored;
        auto channel = std::make_shared<detail::Channel>();
        channel->output = [&](const Json& value) {
            legacy.push_back(value);
        };
        registry.dispatch(
            {{"jsonrpc", "2.0"},
             {"id", 1},
             {"method", "initialize"},
             {"params",
              {{"protocolVersion", mcp::protocolVersion2025},
               {"capabilities", Json::object()},
               {"clientInfo", {{"name", "test"}, {"version", "1"}}}}}},
            mcp::ProtocolEra::Initialization20251125,
            [](const Json&, bool) {
            },
            utils::Timeval(30),
            {},
            channel);
        const auto listen = [&](int id, Json filter, std::vector<Json>& messages) {
            return registry.dispatch({{"jsonrpc", "2.0"},
                                      {"id", id},
                                      {"method", "subscriptions/listen"},
                                      {"params",
                                       {{"_meta", {{mcp::versionKey, mcp::protocolVersion2026}, {mcp::capabilitiesKey, Json::object()}}},
                                        {"notifications", filter}}}},
                                     mcp::ProtocolEra::PerRequest20260728,
                                     [&messages](const Json& value, bool) {
                                         if (!value.is_null()) {
                                             messages.push_back(value);
                                         }
                                     });
        };
        auto opted = listen(2, {{"toolsListChanged", true}, {"resourcesListChanged", true}, {"promptsListChanged", true}}, modern);
        auto unrequested = listen(3, Json::object(), ignored);
        const Json schema{{"type", "object"}};
        const auto handler = [](const auto&, const auto& reply) {
            reply->result(mcp::ToolResult::text("done"));
        };
        server.tool({"one", {}, schema}, handler);
        server.tool({"two", {}, schema}, handler);
        server.remove(mcp::server::RegistrationKind::Tool, "one");
        server.resource({"test://one", "one"}, [](const auto&, const auto& reply) {
            reply->result(mcp::ResourceResult{});
        });
        server.resourceTemplate({"test://{name}", "template"}, [](const auto&, const auto& reply) {
            reply->result(mcp::ResourceResult{});
        });
        server.prompt({"prompt", {}, {}}, [](const auto&, const auto& reply) {
            reply->result(mcp::PromptResult{});
        });
        test.expectTrue(legacy.empty() && modern.size() == 1, "mutations do not notify synchronously");
        auto check = core::timer::Timer::singleshotTimer(
            [&] {
                test.expectTrue(legacy.size() == 3 && modern.size() == 4 && ignored.size() == 1,
                                "bursts coalesce once per category, templates share the resource category, opt-in enforced");
                for (const auto& value : legacy) {
                    test.expectTrue(!value.contains("params"), "legacy list-change notification has no modern metadata");
                }
                for (std::size_t i = 1; i < modern.size(); ++i) {
                    test.expectTrue(modern[i]["params"]["_meta"][mcp::subscriptionKey] == 2,
                                    "modern list-change carries its subscription id");
                }
                opted->cancel();
                unrequested->cancel();
                channel->active = false;
                server.remove(mcp::server::RegistrationKind::Tool, "two");
            },
            utils::Timeval(0.002));
        auto stop = core::timer::Timer::singleshotTimer(
            [] {
                core::SNodeC::stop();
            },
            utils::Timeval(0.005));
        core::SNodeC::start(utils::Timeval(1));
        test.expectTrue(legacy.size() == 3 && modern.size() == 4, "closed consumers receive no change notification");
    }
    core::SNodeC::free();
    return test.processResult();
}
