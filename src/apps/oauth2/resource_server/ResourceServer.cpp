#include "express/legacy/in/WebApp.h"
#include "express/middleware/JsonMiddleware.h"
#include "log/Logger.h"
#include "web/http/client/Request.h"  // for Request, client
#include "web/http/client/Response.h" // for Response
#include "web/http/legacy/in/Client.h"

#include <map>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <string>

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);

    express::legacy::in::WebApp app("OAuth2ResourceServer");

    const std::string authorizationServerUri{"http://localhost:8082"};

    app.use(express::middleware::JsonMiddleware());

    app.get("/access", [authorizationServerUri] APPLICATION(req, res) {
        res.set("Access-Control-Allow-Origin", "*");
        std::string queryAccessToken{req.query("access_token")};
        std::string queryClientId{req.query("client_id")};
        if (queryAccessToken.empty() || queryClientId.empty()) {
            VLOG(0) << "Missing access_token or client_id in body";
            res.sendStatus(401);
            return;
        }

        web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response> legacyClient(
            [](web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>::SocketConnection* socketConnection)
                -> void {
                VLOG(0) << "OnConnect";

                VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
                VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
            },
            []([[maybe_unused]] web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>::SocketConnection*
                   socketConnection) -> void {
                VLOG(0) << "OnConnected";
            },
            [queryAccessToken, queryClientId](web::http::client::Request& request) -> void {
                VLOG(0) << "OnRequestBegin";
                request.url = "/oauth2/token/validate?client_id=" + queryClientId;
                request.method = "POST";
                VLOG(0) << "ClientId: " << queryClientId;
                VLOG(0) << "AcceessToken: " << queryAccessToken;
                nlohmann::json requestJson = {{"access_token", queryAccessToken}, {"client_id", queryClientId}};
                std::string requestJsonString{requestJson.dump(4)};
                request.send(requestJsonString);
            },
            [&res]([[maybe_unused]] web::http::client::Request& request, web::http::client::Response& response) -> void {
                VLOG(0) << "OnResponse";
                response.body.push_back(0);
                VLOG(0) << "Response: " << response.body.data();
                if (std::stoi(response.statusCode) != 200) {
                    nlohmann::json errorJson = {{"error", "Invalid access token"}};
                    res.status(401).send(errorJson.dump(4));
                } else {
                    nlohmann::json successJson = {{"content", "🦆"}};
                    res.status(200).send(successJson.dump(4));
                }
            },
            [](int status, const std::string& reason) -> void {
                VLOG(0) << "OnResponseError";
                VLOG(0) << "     Status: " << status;
                VLOG(0) << "     Reason: " << reason;
            },
            [](web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>::SocketConnection* socketConnection)
                -> void {
                VLOG(0) << "OnDisconnect";

                VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
                VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
            });

        legacyClient.connect(
            "localhost",
            8082,
            [](const web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>::SocketAddress& socketAddress,
               core::socket::State state) -> void {
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

    app.listen(8083, [](const express::legacy::in::WebApp::SocketAddress socketAddress, const core::socket::State& state) {
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
