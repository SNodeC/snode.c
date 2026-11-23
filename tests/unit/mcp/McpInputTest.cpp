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
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <nlohmann/json.hpp>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace mcp::server;

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    const Json schema = nlohmann::ordered_json::parse(R"json(
{
    "type": "object"
}
)json");
    Server server("inputs", "1");
    server.tool({"input", {}, schema}, [](const auto& request, const auto& reply) {
        if (request.context.getInputResponses().empty()) {
            reply->inputRequired(InputRequired{{{"choice", UrlInput{"Choose", "https://example.com/choice"}}}, "state"});
        } else {
            reply->result(ToolResult::text(*request.context.getRequestState()));
        }
    });
    const auto dispatch = [&](detail::ProtocolEra era, Json params) {
        params["name"] = "input";
        Json result;
        detail::Access::registry(server).dispatch(
            {{"jsonrpc", "2.0"}, {"id", 1}, {"method", "tools/call"}, {"params", params}}, era, [&](const Json& value, bool) {
                result = value;
            });

        return result;
    };
    Json params{
        {"_meta",
         {{detail::versionKey, detail::protocolVersion2026}, {detail::capabilitiesKey, {{"elicitation", {{"url", Json::object()}}}}}}}};
    const auto response = dispatch(detail::ProtocolEra::PerRequest20260728, params);
    test.expectTrue(response["result"]["resultType"] == "input_required" && response["result"]["requestState"] == "state" &&
                        response["result"]["inputRequests"]["choice"]["params"]["mode"] == "url",
                    "typed MRTR URL result");
    params["inputResponses"] = {{"choice", {{"action", "accept"}}}};
    params["requestState"] = "echoed";
    test.expectTrue(dispatch(detail::ProtocolEra::PerRequest20260728, params)["result"]["content"][0]["text"] == "echoed",
                    "retry input/state reaches handler");
    params.erase("inputResponses");
    params.erase("requestState");
    params["_meta"][detail::capabilitiesKey] = Json::object();
    test.expectTrue(dispatch(detail::ProtocolEra::PerRequest20260728, params)["error"]["code"] == -32021,
                    "missing mode capability rejected");
    test.expectTrue(dispatch(detail::ProtocolEra::Initialization20251125, Json::object())["error"]["code"] == -32602,
                    "legacy continuation explicitly unsupported");
    test.expectTrue(dispatch(detail::ProtocolEra::Initialization20251125,
                             {{"inputResponses", {{"choice", {{"action", "accept"}}}}}, {"requestState", "injected"}})["error"]["code"] ==
                        -32602,
                    "legacy requests cannot inject modern continuation fields");
    core::SNodeC::free();

    return test.processResult();
}
