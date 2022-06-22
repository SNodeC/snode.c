#include "express/legacy/in/WebApp.h"
#include "express/middleware/JsonMiddleware.h"
#include "web/http/client/Request.h"  // for Request, client
#include "web/http/client/Response.h" // for Response
#include "web/http/legacy/in/Client.h"

#include <memory>
#include <nlohmann/json.hpp>
#include <vector>

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);
    express::legacy::in::WebApp app{"OAuth2ResourceServer"};

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

        /*
web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response> legacyClient(
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
    [bodyAccessToken](web::http::client::Request& request) -> void {
        VLOG(0) << "OnRequestBegin";
        nlohmann::json requestJson = {{"access_token", bodyAccessToken}};
        request.start();
    },
    [&res]([[maybe_unused]] web::http::client::Request& request, web::http::client::Response& response) -> void {
        VLOG(0) << "OnResponse";
        if (std::stoi(response.statusCode) == 200) {
            nlohmann::json errorJson = {{"error", "Invalid access token"}};
            res.status(401).send(errorJson);
        } else {
            nlohmann::json successJson = {{"content", "🦆"}};
             es.status(200).send(successJson);


      (int status, const std::string& reason) -> void {
         LOG(0) << "OnResponseError";
          OG(0) << "     Status: " << status;
          OG(0) << "     Reason: " << reason;
     ,
     ](web::http::legacy::in::Client<web::http::client::Request, web::http::client::Response>::SocketConnection*
             cketConnection) -> void {
         LOG(0) << "OnDisconnect";
          OG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
          OG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
    });

  gacyClient.connect("localhost",
                       82,
                      ](const web::http::legacy::in::Client<web::http::client::Request,
                                                              b::http::client::Response>::SocketAddress& socketAddress,
                          t err) -> void {
                          f (err != 0) {
                               OG(ERROR) << "OnError: " << err;
                           else {
                             VLOG(0) << "Client connecting to " << socketAddress.toString();

                       ;
                                     */
        nlohmann::json successJson = {{"content", "🦆"}};
        VLOG(0) << "Sending response:  " << successJson.dump(2);
        res.status(200).send(successJson.dump(4));
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
