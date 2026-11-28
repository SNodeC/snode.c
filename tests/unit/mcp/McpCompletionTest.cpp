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
#include "mcp/server/Request.h"
#include "mcp/server/Results.h"
#include "mcp/server/Server.h"
#include "mcp/server/detail/Server.h"
#include "tests/support/TestResult.h"

#include <cmath>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <initializer_list>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace mcp::server;

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);
    tests::support::TestResult test;
    Server server("completion", "1");
    bool contextPassed = false;
    const CompletionHandler complete = [&](const CompletionRequest& request, const std::shared_ptr<CompletionReply>& reply) {
        contextPassed = request.arguments.at("language") == "C++" && request.value == "Soc";
        reply->result(CompletionResult{std::vector<std::string>(101, "SocketContext"), 110, false});
    };
    server.prompt({"review", {}, {{"header", {}, Requirement::Optional, complete}}}, [](const auto&, const auto& reply) {
        reply->result(PromptResult{});
    });
    server.resourceTemplate({"test://{header}", "headers", {}, {}, {}, {{"header", complete}}}, [](const auto&, const auto& reply) {
        reply->result(ResourceResult{});
    });
    for (const auto era : {detail::ProtocolEra::Initialization20251125, detail::ProtocolEra::PerRequest20260728}) {
        for (const std::string identifier : {"review", "test://{header}", "test://SocketContext"}) {
            const std::string type = identifier == "review" ? "ref/prompt" : "ref/resource";
            Json params{{"ref", {{"type", type}, {type == "ref/prompt" ? "name" : "uri", identifier}}},
                        {"argument", {{"name", "header"}, {"value", "Soc"}}},
                        {"context", {{"arguments", {{"language", "C++"}}}}}};
            if (era == detail::ProtocolEra::PerRequest20260728) {
                params["_meta"] = {{detail::versionKey, detail::protocolVersion2026}, {detail::capabilitiesKey, Json::object()}};
            }

            Json result;
            detail::Access::registry(server).dispatch(
                {{"jsonrpc", "2.0"}, {"id", 3}, {"method", "completion/complete"}, {"params", params}}, era, [&](const Json& value, bool) {
                    result = value;
                });
            const auto completion = result["result"]["completion"];
            test.expectTrue(contextPassed && completion["values"].size() == 100 && completion["hasMore"] == true &&
                                completion["total"] == 110,
                            "completers receive context and enforce 100-item result limit in both eras");
        }
    }

    core::SNodeC::free();

    return test.processResult();
}
