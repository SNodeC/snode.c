#include <SemanticLog.h>
#include "express/legacy/in/WebApp.h"
#include "express/middleware/JsonMiddleware.h"
#include "log/Logger.h"
#include "web/http/client/Request.h"  // for Request, client
#include "web/http/client/Response.h" // for Response
#include "web/http/legacy/in/Client.h"

#include <iostream>
#include <nlohmann/json.hpp> // IWYU pragma: keep

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);

    express::legacy::in::WebApp app;

    const std::string authorizationServerUri{"http://localhost:8082"};

    app.use(express::middleware::JsonMiddleware());

    app.get("/access", [authorizationServerUri] APPLICATION(req, res) {
        res.set("Access-Control-Allow-Origin", "*");
        std::string queryAccessToken{req.query("access_token")};
        std::string queryClientId{req.query("client_id")};
        if (queryAccessToken.empty() || queryClientId.empty()) {
            snode::semantic::appLog().trace() << "Missing access_token or client_id in body";
            res.sendStatus(401);
            return;
        }

        web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response> legacyClient(
            [](web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>::SocketConnection* socketConnection)
                -> void {
                snode::semantic::appLog().trace() << "OnConnect";

                snode::semantic::appLog().trace() << "\tServer: " + socketConnection->getRemoteAddress().toString();
                snode::semantic::appLog().trace() << "\tClient: " + socketConnection->getLocalAddress().toString();
            },
            []([[maybe_unused]] web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>::SocketConnection*
                   socketConnection) -> void {
                snode::semantic::appLog().trace() << "OnConnected";
            },
            [queryAccessToken, queryClientId](web::http::client::Request& request) -> void {
                snode::semantic::appLog().trace() << "OnRequestBegin";
                request.url = "/oauth2/token/validate?client_id=" + queryClientId;
                request.method = "POST";
                snode::semantic::appLog().trace() << "ClientId: " << queryClientId;
                snode::semantic::appLog().trace() << "AcceessToken: " << queryAccessToken;
                nlohmann::json requestJson = {{"access_token", queryAccessToken}, {"client_id", queryClientId}};
                std::string requestJsonString{requestJson.dump(4)};
                request.send(requestJsonString);
            },
            [&res]([[maybe_unused]] web::http::client::Request& request, web::http::client::Response& response) -> void {
                snode::semantic::appLog().trace() << "OnResponse";
                response.body.push_back(0);
                snode::semantic::appLog().trace() << "Response: " << response.body.data();
                if (std::stoi(response.statusCode) != 200) {
                    nlohmann::json errorJson = {{"error", "Invalid access token"}};
                    res.status(401).send(errorJson.dump(4));
                } else {
                    nlohmann::json successJson = {{"content", "🦆"}};
                    res.status(200).send(successJson.dump(4));
                }
            },
            [](int status, const std::string& reason) -> void {
                snode::semantic::appLog().trace() << "OnResponseError";
                snode::semantic::appLog().trace() << "     Status: " << status;
                snode::semantic::appLog().trace() << "     Reason: " << reason;
            },
            [](web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>::SocketConnection* socketConnection)
                -> void {
                snode::semantic::appLog().trace() << "OnDisconnect";

                snode::semantic::appLog().trace() << "\tServer: " + socketConnection->getRemoteAddress().toString();
                snode::semantic::appLog().trace() << "\tClient: " + socketConnection->getLocalAddress().toString();
            });

        legacyClient.connect(
            "localhost",
            8082,
            [](const web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>::SocketAddress& socketAddress,
               int err) -> void {
                if (err != 0) {
                    snode::semantic::sysError(snode::semantic::appLog(), logger::LogLevel::Error, errno) << "OnError: " << err;
                } else {
                    snode::semantic::appLog().trace() << "Resource server client connecting to " << socketAddress.toString();
                }
            });
    });

    app.listen(8083, [](const express::legacy::in::WebApp::SocketAddress socketAddress, int err) {
        if (err != 0) {
            std::cerr << "Failed to listen on port " << 8083 << std::endl;
        } else {
            std::cout << "OAuth2ResourceServer is listening on " << socketAddress.toString() << std::endl;
        }
    });
    return express::WebApp::start();
}
