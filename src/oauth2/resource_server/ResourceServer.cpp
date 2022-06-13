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
