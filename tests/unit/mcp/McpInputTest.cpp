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
#include <initializer_list>
#include <map>
#include <memory>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
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
    server.resource({"test://input", "input"}, [](const auto&, const auto& reply) {
        reply->inputRequired(InputRequired{{}, "resource-state"});
    });
    server.prompt({"input"}, [](const auto&, const auto& reply) {
        reply->inputRequired(InputRequired{{}, "prompt-state"});
    });
    const Json form = {{"type", "object"}, {"properties", {{"label", {{"type", "string"}}}}}};
    int resumes = 0;
    server.tool({"form", {}, schema}, [&resumes, form](const auto& request, const auto& reply) {
        if (request.context.getInputResponses().empty()) {
            InputRequired input{{{"label", FormInput{"Label", form}}}, "saved"};
            if (request.arguments.value("batch", false)) {
                input.requests.emplace("url", UrlInput{"Choose", "https://example.com/choice"});
            }

            reply->inputRequired(std::move(input));
        } else {
            ++resumes;
            if (request.arguments.value("throw", false)) {
                throw 1;
            }

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
    test.expectTrue(dispatch(detail::ProtocolEra::Initialization20251125, Json::object())["result"]["isError"] == true,
                    "legacy client without elicitation gets a tool error");
    test.expectTrue(dispatch(detail::ProtocolEra::Initialization20251125,
                             {{"inputResponses", {{"choice", {{"action", "accept"}}}}}, {"requestState", "injected"}})["error"]["code"] ==
                        -32602,
                    "legacy requests cannot inject modern continuation fields");
    for (const auto era : {detail::ProtocolEra::Initialization20251125, detail::ProtocolEra::PerRequest20260728}) {
        for (const std::string method : {"resources/read", "prompts/get"}) {
            Json response;
            const Json parameters = method == "resources/read" ? Json{{"uri", "test://input"}} : Json{{"name", "input"}};
            Json request{{"jsonrpc", "2.0"}, {"id", 2}, {"method", method}, {"params", parameters}};
            if (era == detail::ProtocolEra::PerRequest20260728) {
                request["params"]["_meta"] = {{detail::versionKey, detail::protocolVersion2026}, {detail::capabilitiesKey, Json::object()}};
            }

            detail::Access::registry(server).dispatch(request, era, [&](const Json& value, bool) {
                response = value;
            });
            test.expectTrue(era == detail::ProtocolEra::PerRequest20260728
                                ? response["result"]["resultType"] == "input_required" && response["result"]["requestState"].is_string()
                                : response["error"]["code"] == -32602,
                            "resource and prompt continuation handles respect the protocol era");
        }
    }
    auto channel = std::make_shared<detail::Channel>();
    std::vector<Json> output;
    int identifier = 10;
    const auto legacy = [&](const std::string& method, Json parameters, const utils::Timeval& timeout = utils::Timeval(30)) {
        return detail::Access::registry(server).dispatch(
            {{"jsonrpc", "2.0"}, {"id", ++identifier}, {"method", method}, {"params", parameters}},
            detail::ProtocolEra::Initialization20251125,
            [&output](const Json& value, bool) {
                output.push_back(value);
            },
            timeout,
            {},
            channel);
    };
    legacy("initialize",
           {{"protocolVersion", "2025-11-25"},
            {"clientInfo", {{"name", "input-test"}, {"version", "1"}}},
            {"capabilities", {{"elicitation", Json::object()}}}});
    test.expectTrue(channel->capabilities["elicitation"].empty(), "initialize retains form-only capabilities");
    for (const std::string action : {"accept", "decline", "cancel"}) {
        output.clear();
        const auto pending = legacy("tools/call", {{"name", "form"}});
        test.expectTrue(!pending->done && output.size() == 1 && output[0]["method"] == "elicitation/create" &&
                            output[0]["params"]["mode"] == "form",
                        "legacy emits a nested form request");
        const Json response = {
            {"jsonrpc", "2.0"}, {"id", output[0]["id"]}, {"result", {{"action", action}, {"content", {{"label", "safe"}}}}}};
        test.expectTrue(channel->respond(response) && pending->done && output.back()["id"] == identifier &&
                            output.back()["result"]["content"][0]["text"] == "saved",
                        "legacy action resumes the original request with server-held state");
        test.expectTrue(!channel->respond(response), "duplicate elicitation reply is ignored");
    }
    channel->capabilities["elicitation"]["url"] = Json::object();
    channel->capabilities["elicitation"]["form"] = Json::object();
    output.clear();
    auto batch = legacy("tools/call", {{"name", "form"}, {"arguments", {{"batch", true}}}});
    test.expectTrue(output.size() == 2 && output[1]["params"]["elicitationId"].is_string(), "URL requests include a legacy elicitationId");
    const auto answer = [](const Json& id) {
        return Json{{"jsonrpc", "2.0"}, {"id", id}, {"result", {{"action", "accept"}}}};
    };
    const Json first = output[0]["id"];
    const Json second = output[1]["id"];
    const int before = resumes;
    test.expectTrue(channel->respond(answer(second)) && !batch->done && resumes == before, "out-of-order partial batch does not resume");
    test.expectTrue(channel->respond(answer(first)) && batch->done && resumes == before + 1 && channel->inputs.empty(),
                    "batch resumes exactly once");
    output.clear();
    auto cancelled = legacy("tools/call", {{"name", "form"}});
    const Json late = answer(output[0]["id"]);
    test.expectTrue(!channel->respond({{"jsonrpc", "2.0"}, {"id", output[0]["id"]}, {"result", {{"action", "invalid"}}}}) &&
                        !cancelled->done,
                    "malformed action cannot resume a handler");
    cancelled->cancel();
    test.expectTrue(!channel->respond(late) && channel->inputs.empty() && resumes == before + 1,
                    "cancellation releases correlations and rejects late replies");
    output.clear();
    auto failed = legacy("tools/call", {{"name", "form"}});
    test.expectTrue(
        channel->respond({{"jsonrpc", "2.0"}, {"id", output[0]["id"]}, {"error", {{"code", -32602}, {"message", "unsupported"}}}}) &&
            failed->done && output.back()["result"]["isError"] == true,
        "client RPC error becomes an ordinary tool error");
    output.clear();
    auto throwing = legacy("tools/call", {{"name", "form"}, {"arguments", {{"throw", true}}}});
    test.expectTrue(channel->respond(answer(output[0]["id"])) && throwing->done && output.back()["error"]["code"] == -32603,
                    "resumed handler exceptions are contained");
    output.clear();
    auto deadline = legacy("tools/call", {{"name", "form"}});
    const Json expired = answer(output[0]["id"]);
    deadline->deadline = utils::Timeval(0);
    test.expectTrue(!channel->respond(expired) && deadline->done && output.back()["result"]["isError"] == true && channel->inputs.empty(),
                    "deadline expires before a late client reply can resume");
    core::SNodeC::free();

    return test.processResult();
}
