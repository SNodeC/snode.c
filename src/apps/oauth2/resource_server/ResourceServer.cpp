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

#include "express/legacy/in/WebApp.h"
#include "express/middleware/JsonMiddleware.h"
#include "log/Logger.h"
#include "web/http/legacy/in/Client.h"

#include <nlohmann/json.hpp>
#include <string>

// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);

    const express::legacy::in::WebApp app("OAuth2ResourceServer");

    const std::string authorizationServerUri{"http://localhost:8082"};

    app.use(express::middleware::JsonMiddleware());

    app.get("/access", [authorizationServerUri] APPLICATION(req, res) {
        res->set("Access-Control-Allow-Origin", "*");
        const std::string queryAccessToken{req->query("access_token")};
        const std::string queryClientId{req->query("client_id")};
        if (queryAccessToken.empty() || queryClientId.empty()) {
            VLOG(1) << "Missing access_token or client_id in body";
            res->sendStatus(401);
            return;
        }

        const web::http::legacy::in::Client legacyClient(
            [](web::http::legacy::in::Client::SocketConnection* socketConnection) {
                VLOG(1) << "OnConnect";

                VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
                VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();
            },
            []([[maybe_unused]] web::http::legacy::in::Client::SocketConnection* socketConnection) {
                VLOG(1) << "OnConnected";
            },
            [](web::http::legacy::in::Client::SocketConnection* socketConnection) {
                VLOG(1) << "OnDisconnect";

                VLOG(1) << "\tServer: " + socketConnection->getRemoteAddress().toString();
                VLOG(1) << "\tClient: " + socketConnection->getLocalAddress().toString();
            },
            [queryAccessToken, queryClientId, res](const std::shared_ptr<web::http::client::MasterRequest>& request) {
                VLOG(1) << "OnRequestBegin";
                request->url = "/oauth2/token/validate?client_id=" + queryClientId;
                request->method = "POST";
                VLOG(1) << "ClientId: " << queryClientId;
                VLOG(1) << "AccessToken: " << queryAccessToken;
                const nlohmann::json requestJson = {{"access_token", queryAccessToken}, {"client_id", queryClientId}};
                const std::string requestJsonString{requestJson.dump(4)};
                request->send(
                    requestJsonString,
                    [res]([[maybe_unused]] const std::shared_ptr<web::http::client::Request>& request,
                          const std::shared_ptr<web::http::client::Response>& response) {
                        VLOG(1) << "OnResponse";
                        VLOG(1) << "Response: " << std::string(response->body.begin(), response->body.end());
                        if (std::stoi(response->statusCode) != 200) {
                            const nlohmann::json errorJson = {{"error", "Invalid access token"}};
                            res->status(401).send(errorJson.dump(4));
                        } else {
                            const nlohmann::json successJson = {{"content", "🦆"}};
                            res->status(200).send(successJson.dump(4));
                        }
                    },
                    [](const std::shared_ptr<web::http::client::Request>&, const std::string& message) {
                        VLOG(1) << "OAuth2ResourceServer: Request parse error: " << message;
                    });
            },
            []([[maybe_unused]] const std::shared_ptr<web::http::client::Request>& req) {
                LOG(INFO) << " -- OnRequestEnd";
            });

        legacyClient.connect(
            "localhost", 8082, [](const web::http::legacy::in::Client::SocketAddress& socketAddress, const core::socket::State& state) {
                switch (state) {
                    case core::socket::State::OK:
                        VLOG(1) << "OAuth2ResourceServer: connected to '" << socketAddress.toString() << "'";
                        break;
                    case core::socket::State::DISABLED:
                        VLOG(1) << "OAuth2ResourceServer: disabled";
                        break;
                    case core::socket::State::ERROR:
                        VLOG(1) << "OAuth2ResourceServer: error occurred";
                        break;
                    case core::socket::State::FATAL:
                        VLOG(1) << "OAuth2ResourceServer: fatal error occurred";
                        break;
                }
            });
    });

    app.listen(8083, [](const express::legacy::in::WebApp::SocketAddress& socketAddress, const core::socket::State& state) {
        switch (state) {
            case core::socket::State::OK:
                VLOG(1) << "app: listening on '" << socketAddress.toString() << "'";
                break;
            case core::socket::State::DISABLED:
                VLOG(1) << "app: disabled";
                break;
            case core::socket::State::ERROR:
                VLOG(1) << "app: error occurred";
                break;
            case core::socket::State::FATAL:
                VLOG(1) << "app: fatal error occurred";
                break;
        }
    });
    return express::WebApp::start();
}
