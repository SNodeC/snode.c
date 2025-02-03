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
            [queryAccessToken, queryClientId, res](const std::shared_ptr<web::http::client::Request>& request) {
                VLOG(1) << "OnRequestBegin";
                request->url = "/oauth2/token/validate?client_id=" + queryClientId;
                request->method = "POST";
                VLOG(1) << "ClientId: " << queryClientId;
                VLOG(1) << "AccessToken: " << queryAccessToken;
                const nlohmann::json requestJson = {{"access_token", queryAccessToken}, {"client_id", queryClientId}};
                const std::string requestJsonString{requestJson.dump(4)};
                request->send(requestJsonString,
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
                              });
            },
            []([[maybe_unused]] const std::shared_ptr<web::http::client::Request>& req) {
                LOG(INFO) << " -- OnRequestEnd";
            });

        legacyClient.connect(
            "localhost",
            8082,
            [](const web::http::legacy::in::Client::SocketAddress& socketAddress, const core::socket::State& state) {
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
