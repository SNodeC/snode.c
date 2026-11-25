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
#include "mcp/server/Reply.h"
#include "mcp/server/Results.h"
#include "mcp/server/Server.h"
#include "mcp/server/Types.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"
#include "utils/Timeval.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace mcp::server;

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    Server server("updates", "1");
    server.resource({"test://one", "one"}, [](const auto&, const auto& reply) {
        reply->result(ResourceResult{});
    });
    std::vector<Json> messages;
    const auto output = [&](const Json& value, bool) {
        if (!value.is_null()) {
            messages.push_back(value);
        }
    };
    Json params{{"_meta", {{detail::versionKey, detail::protocolVersion2026}, {detail::capabilitiesKey, Json::object()}}},
                {"notifications", {{"resourceSubscriptions", {"test://one"}}, {"resourcesListChanged", true}}}};
    auto subscription = detail::Access::registry(server).dispatch(
        {{"jsonrpc", "2.0"}, {"id", "watch"}, {"method", "subscriptions/listen"}, {"params", params}},
        detail::ProtocolEra::PerRequest20260728,
        output);
    test.expectTrue(messages.size() == 1 && messages[0]["method"] == "notifications/subscriptions/acknowledged" &&
                        messages[0]["params"]["_meta"][detail::subscriptionKey] == "watch" &&
                        messages[0]["params"]["notifications"]["resourcesListChanged"] == true,
                    "ack first, supported list-change filter honored, typed subscription ID");
    server.resourceUpdated("test://other");
    test.expectTrue(messages.size() == 1, "unrequested URI suppressed");
    server.resourceUpdated("test://one");
    test.expectTrue(messages.size() == 2 && messages.back()["method"] == "notifications/resources/updated" &&
                        messages.back()["params"]["_meta"][detail::subscriptionKey] == "watch",
                    "requested update correlated");
    subscription->cancel();
    server.resourceUpdated("test://one");
    test.expectTrue(messages.size() == 2, "cancelled subscription removed");
    auto channel = std::make_shared<detail::Channel>();
    channel->output = [&](const Json& value) {
        messages.push_back(value);
    };
    const auto legacy = [&](const std::string& method) {
        Json result;
        detail::Access::registry(server).dispatch(
            {{"jsonrpc", "2.0"}, {"id", 2}, {"method", method}, {"params", {{"uri", "test://one"}}}},
            detail::ProtocolEra::Initialization20251125,
            [&](const Json& value, bool) {
                result = value;
            },
            utils::Timeval(30),
            {},
            channel);

        return result;
    };
    test.expectTrue(legacy("resources/subscribe").contains("result"), "legacy subscribe accepted");
    server.resourceUpdated("test://one");
    test.expectTrue(messages.size() == 3 && !messages.back()["params"].contains("_meta"),
                    "legacy update has no modern subscription metadata");
    legacy("resources/unsubscribe");
    server.resourceUpdated("test://one");
    test.expectTrue(messages.size() == 3, "legacy unsubscribe releases interest");
    core::SNodeC::free();

    return test.processResult();
}
