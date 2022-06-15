#include "express/legacy/in/WebApp.h"
#include "express/middleware/JsonMiddleware.h"

#include <memory>
#include <nlohmann/json.hpp>
#include <vector>

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);
    express::legacy::in::WebApp app{"OAuth2ResourceServer"};

    const std::string authorizationServerUri{"http://localhost:8082"};
    std::shared_ptr<std::vector<std::string>> clientIds;

    app.use(express::middleware::JsonMiddleware());

    app.get("/access", [authorizationServerUri, clientIds] APPLICATION(req, res) {
        req.getAttribute<nlohmann::json>([&req, &res, authorizationServerUri, clientIds](nlohmann::json& body) -> void {
            std::string bodyAccessToken{body["access_token"]};
            std::string bodyClientId{body["client_id"]};
            if (bodyAccessToken.empty() || bodyClientId.empty()) {
                VLOG(0) << "Missing access_token or client_id in body";
                res.sendStatus(401);
                return;
            }
            /*
            web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response> legacyClient(
                "legacy",
                [](web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>::SocketConnection*
                       socketConnection) -> void {
                    VLOG(0) << "OnConnect";

                    VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
                    VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
                },
                []([[maybe_unused]] web::http::legacy::in::Client<web::http::client::Request,
                                                                  web::http::client::Response>::SocketConnection* socketConnection)
                    -> void {
                    VLOG(0) << "OnConnected";
                },
                [](web::http::client::Request& request) -> void {
                    VLOG(0) << "OnRequestBegin";

                    request.set("Sec-WebSocket-Protocol", "test, echo");

                    request.upgrade("/ws/", "websocket");
                },
                [](web::http::client::Request& request, web::http::client::Response& response) -> void {
                    VLOG(0) << "OnResponse";
                    VLOG(0) << "     Status:";
                    VLOG(0) << "       " << response.httpVersion << " " << response.statusCode << " " << response.reason;

                    VLOG(0) << "     Headers:";
                    for (const auto& [field, value] : response.headers) {
                        VLOG(0) << "       " << field + " = " + value;
                    }

                    VLOG(0) << "     Cookies:";
                    for (const auto& [name, cookie] : response.cookies) {
                        VLOG(0) << "       " + name + " = " + cookie.getValue();
                        for (const auto& [option, value] : cookie.getOptions()) {
                            VLOG(0) << "         " + option + " = " + value;
                        }
                    }

                    response.body.push_back(0); // make it a c-string
                    VLOG(0) << "Body:\n----------- start body -----------\n"
                            << response.body.data() << "\n------------ end body ------------";

                    response.upgrade(request);
                },
                [](int status, const std::string& reason) -> void {
                    VLOG(0) << "OnResponseError";
                    VLOG(0) << "     Status: " << status;
                    VLOG(0) << "     Reason: " << reason;
                },
                [](web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>::SocketConnection*
                       socketConnection) -> void {
                    VLOG(0) << "OnDisconnect";

                    VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
                    VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
                });
                */
            // apps/http/models/client.h
            // apps/http/websocketclient
            clientIds->push_back(bodyClientId);
            res.redirect(authorizationServerUri + "?client_id" + bodyClientId + "&access_token=" + bodyAccessToken);
        });
    });

    app.post("/accepted", [clientIds] APPLICATION(req, res) {
        std::string queryClientId{req.query("client_id")};
        if (queryClientId.empty() || std::find(clientIds->begin(), clientIds->end(), queryClientId) == clientIds->end()) {
            res.sendStatus(401);
            return;
        }
        VLOG(0) << "Authorization granted to client '" << queryClientId << "'";
        res.send("Authorization granted to client '" + queryClientId + "'");
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
