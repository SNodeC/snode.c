#include <iostream>

#include "express/legacy/in/WebApp.h"
#include "express/Router.h"
#include "express/middleware/StaticMiddleware.h"
#include "express/middleware/VHost.h"
#include "express/middleware/JsonMiddleware.h"
#include <vector>
#include <nlohmann/json.hpp>

/*
#include <openssl/sha.h>
string sha256(const string str)
{
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
    {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}
*/

void addQueryParamToUri(std::string& uri, std::string queryParamName, std::string queryParamValue) {
    if (uri.find("?") == std::string::npos) {
        uri += "?";
    } else {
        uri += "&";
    }
    uri += queryParamName + "=" + queryParamValue;
}

int main(int argc, char *argv[]) {
    express::WebApp::init(argc, argv);
    express::legacy::in::WebApp app{"OAuth2AuthorizationServer"};

    const std::string emailDemoUser{"daniel@flockert.at"};
    const std::string pwDemoUser{"rathalin"};
    std::string clientRedirectUri{""};
    std::string clientId{""};
    std::string clientState{""};
    std::string authCode{"abc"};
    std::string accessToken{"ghj"};
    std::string refreshToken{"tzu"};

    app.get("/auth", [&clientRedirectUri, &clientId, &clientState] APPLICATION(req, res) {
        VLOG(0) << "Query params:";
        auto paramResponseType = req.query("response_type");
        VLOG(0) << "ResponseType: " << paramResponseType;
        auto paramRedirectUri = req.query("redirect_uri");
        clientRedirectUri = paramRedirectUri;
        VLOG(0) << "RedirectUri: " << paramRedirectUri;
        auto paramClientId = req.query("client_id");
        clientId = paramClientId;
        VLOG(0) << "ClientId" << paramClientId;
        auto paramState = req.query("state");
        clientState = paramState;
        VLOG(0) << "State: " << paramState;
        VLOG(0) << '\n';
        if (paramResponseType != "code") {
            VLOG(0) << "Auth invalid, sending Bad Request";
            res.sendStatus(400);
            return;
        }
        VLOG(0) << "Auth request valid, redirecting to login";
        res.redirect("/login/index.html");
    });
    
    // app.use("/login", express::middleware::JsonMiddleware());
    app.post("/login", [&emailDemoUser, &pwDemoUser, &clientRedirectUri, &authCode, &clientState, &clientId] APPLICATION(req, res) {
        std::string body = std::string((char *) req.body.data(), req.body.size());
        try
         {
            VLOG(0) << body;
            nlohmann::json json{nlohmann::json::parse(body)};
            VLOG(0) << json;
            std::string paramEmail{json[0].at("email")};
            std::string paramPassword{json[0].at("password")};
            VLOG(0) << "Email: " << paramEmail;
            VLOG(0) << "Password: " << paramPassword;
            if (paramEmail != emailDemoUser || paramPassword != pwDemoUser) {
                VLOG(0) << "Sending unauthorized";
                res.sendStatus(401);
                return;
            }
            std::string redirectUri{clientRedirectUri};
            addQueryParamToUri(clientRedirectUri, "code", authCode);
            if (clientState.length() > 0) {
                addQueryParamToUri(clientRedirectUri, "state", clientState);
            }
            addQueryParamToUri(clientRedirectUri, "client_id", clientId);
            // Set CORS header
            res.set(std::map<std::string, std::string>{
               {"Access-Control-Allow-Origin", "*"},
            });
            VLOG(0) << "Redirecting to " << clientRedirectUri;
            res.redirect(clientRedirectUri);
         }
         catch (std::out_of_range& e)
         {
             VLOG(0) << "out of range: " << e.what();
             res.sendStatus(400);
         }
    });

    app.get("/token", [&accessToken, &refreshToken] APPLICATION(req, res) {
        // Store code as primary key in db
        auto paramGrantType = req.query("grant_type");
        VLOG(0) << "GrandType: " << paramGrantType;
        auto paramCode = req.query("code");
        VLOG(0) << "Code: " << paramCode;
        auto paramRedirectUri = req.query("redirect_uri");
        VLOG(0) << "RedirectUri: " << paramRedirectUri;
        auto paramClientId = req.query("client_id");
        VLOG(0) << "ClientId" << paramClientId;
        if (paramGrantType != "authorization_code" || paramCode.length() == 0 || paramRedirectUri.length() == 0 || paramClientId.length() == 0) {
            VLOG(0) << paramGrantType << ", " << paramCode << ", " << paramRedirectUri << ", " << paramClientId;
            VLOG(0) << "Token request invalid, sending Bad Request";
            res.sendStatus(400);
            return;
        }
        // UUID example: 4a3797ae-0734-4f7f-ad2c-1c1bfafba7da
        // cat /proc/sys/kernel/random/uuid
        // Open file -> readline -> close
        /*
        {
           "access_token":"2YotnFZFEjr1zCsicMWpAA",
           "token_type":"example",
           "expires_in":3600,
           "refresh_token":"tGzv3JOkF0XG5Qx2TlKWIA",
           "example_parameter":"example_value"
         }
        */
        std::string jsonString{
            "{"
                "\"access_token\":\"" "\"" + accessToken +  "\","
                "\"token_type\":\"example\","
                "\"expirsed_in\":3600,"
                "\"refres_token\":\"" + refreshToken + "\""
            "}"
        };
        res.send(jsonString);
    });

    app.use(express::middleware::StaticMiddleware("/home/rathalin/projects/snode.c/src/oauth2/authorization_server/www"));

    app.listen(8082, [] (const express::legacy::in::WebApp::SocketAddress socketAddress, int err) {
        if (err != 0) {
            std::cerr << "Failed to listen on port " << 8082 << std::endl;
        } else {
            std::cout << "OAuth2AuthorizationServer is listening on " << socketAddress.toString() << std::endl;
        }
    });

    return express::WebApp::start();
}
