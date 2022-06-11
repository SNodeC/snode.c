#include "database/mariadb/MariaDBClient.h"
#include "database/mariadb/MariaDBCommandSequence.h"
#include "express/Router.h"
#include "express/legacy/in/WebApp.h"
#include "express/middleware/JsonMiddleware.h"
#include "express/middleware/StaticMiddleware.h"
#include "express/middleware/VHost.h"
#include "utils/sha1.h"

#include <iostream>
#include <nlohmann/json.hpp>
#include <vector>

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

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);
    express::legacy::in::WebApp app{"OAuth2AuthorizationServer"};

    database::mariadb::MariaDBConnectionDetails details{
        .hostname = "localhost",
        .username = "rathalin",
        .password = "rathalin",
        .database = "oauth2",
        .port = 3306,
        .socket = "/run/mysqld/mysqld.sock",
        .flags = 0,
    };
    database::mariadb::MariaDBClient db{details};

    app.use(express::middleware::JsonMiddleware());

    // Middleware to catch requests without a valid client_id
    /*
    app.use([&db] MIDDLEWARE(req, res, next) {
        std::string queryClientId{req.query("client_id")};
        if (queryClientId.length() > 0) {
            db.query(
                "select count(*) from client where uuid = '" + queryClientId + "'",
                [&req, &res, &next, queryClientId](const MYSQL_ROW row) -> void {
                    if (row != nullptr) {
                        if (std::stoi(row[0]) > 0) {
                            VLOG(0) << "Valid client id '" << queryClientId << "'";
                            next();
                        } else {
                            VLOG(0) << "Invalid client id '" << queryClientId << "'";
                            res.sendStatus(401);
                        }
                    }
                },
                [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                    VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                    res.sendStatus(500);
                });
        } else {
            res.sendStatus(401);
        }
    });
    */

    app.get("/authorize", [&db] APPLICATION(req, res) {
        // REQUIRED: response_type, client_id
        // OPTIONAL: redirect_uri, scope
        // RECOMMENDED: state
        std::string paramResponseType{req.query("response_type")};
        std::string paramClientId{req.query("client_id")};
        std::string paramRedirectUri{req.query("redirect_uri")};
        std::string paramScope{req.query("scope")};
        std::string paramState{req.query("state")};

        VLOG(0) << "Query params: "
                << "response_type=" << req.query("response_type") << ", "
                << "redirect_uri=" << req.query("redirect_uri") << ", "
                << "scope=" << req.query("scope") << ", "
                << "state=" << req.query("state") << "\n";

        if (paramResponseType != "code") {
            VLOG(0) << "Auth invalid, sending Bad Request";
            res.sendStatus(400);
            return;
        }

        if (!paramRedirectUri.empty()) {
            db.exec(
                "update client set redirect_uri = '" + paramRedirectUri + "' where uuid = '" + paramClientId + "'",
                [paramRedirectUri]() -> void {
                    VLOG(0) << "Database: Set redirect_uri to " << paramRedirectUri;
                },
                [](const std::string& errorString, unsigned int errorNumber) -> void {
                    VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                });
        }

        if (!paramScope.empty()) {
            db.exec(
                "update client set scope = '" + paramScope + "' where uuid = '" + paramClientId + "'",
                [paramScope]() -> void {
                    VLOG(0) << "Database: Set scope to " << paramScope;
                },
                [](const std::string& errorString, unsigned int errorNumber) -> void {
                    VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                });
        }

        if (!paramState.empty()) {
            db.exec(
                "update client set state = '" + paramState + "' where uuid = '" + paramClientId + "'",
                [paramState]() -> void {
                    VLOG(0) << "Database: Set state to " << paramState;
                },
                [](const std::string& errorString, unsigned int errorNumber) -> void {
                    VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                });
        }

        VLOG(0) << "Auth request valid, redirecting to login";
        std::string loginUri{"/login"};
        addQueryParamToUri(loginUri, "client_id", paramClientId);
        res.redirect(loginUri);
    });

    app.get("/login", [] APPLICATION(req, res) {
        res.sendFile("/home/rathalin/projects/snode.c/src/oauth2/authorization_server/vue-frontend/dist/index.html",
                     [&req](int ret) -> void {
                         if (ret != 0) {
                             PLOG(ERROR) << req.url;
                         }
                     });
    });

    // req.getAttribute<json>([&jsonString](json& json) -> {json.dump(4)},[](const
    // std::string& key) -> {}
    app.post("/login", [&db] APPLICATION(req, res) {
        req.getAttribute<nlohmann::json>(
            [&req, &res, &db](nlohmann::json& json) -> void {
                db.query(
                    "select email, password_hash, password_salt "
                    "from client "
                    "where uuid = '" +
                        req.param("client_id") + "'",
                    [&req, &res, &json](const MYSQL_ROW row) -> void {
                        if (row != nullptr) {
                            std::string email{row[0]};
                            std::string password_hash{row[1]};
                            std::string password_salt{row[2]};
                            std::string paramEmail{json["email"]};
                            std::string paramPassword{json["password"]};

                            VLOG(0) << "param email " << paramEmail;
                            VLOG(0) << "param pw " << paramPassword;

                            // base64.cpp
                            VLOG(0) << sha1("123456789");
                            VLOG(0) << sha1("123456789");
                            VLOG(0) << sha1("123456789");
                            VLOG(0) << sha1("123456789");
                            VLOG(0) << sha1("123456789");
                            VLOG(0) << sha1("123456789");
                            VLOG(0) << sha1("123456789");
                            VLOG(0) << sha1("123456789");
                            /*
                           try {
                               std::string clientRedirectUri = req.query("redirect_uri");
                               addQueryParamToUri(clientRedirectUri, "code", authCode);
                               if (clientState.length() > 0) {
                                   addQueryParamToUri(clientRedirectUri, "state", clientState);
                               }
                               addQueryParamToUri(clientRedirectUri, "client_id", clientId);
                               // Set CORS header
                               res.set("Access-Control-Allow-Origin", "*");
                               VLOG(0) << "Redirecting to " << clientRedirectUri;
                               res.redirect(301, clientRedirectUri);
                           } catch (std::out_of_range& e) {
                               VLOG(0) << "out of range: " << e.what();
                               res.sendStatus(400);
                           }
                            */
                        } else {
                            VLOG(0) << "end of post login db";
                        }
                    },
                    [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                        VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                        res.sendStatus(500);
                    });
            },
            [&res]([[maybe_unused]] const std::string& key) -> void {
                res.sendStatus(400);
            });
    });

    app.get("/token", [&db] APPLICATION(req, res) {
        // Store code as primary key in db
        auto paramGrantType = req.query("grant_type");
        VLOG(0) << "GrandType: " << paramGrantType;
        auto paramCode = req.query("code");
        VLOG(0) << "Code: " << paramCode;
        auto paramRedirectUri = req.query("redirect_uri");
        VLOG(0) << "RedirectUri: " << paramRedirectUri;
        auto paramClientId = req.query("client_id");
        VLOG(0) << "ClientId" << paramClientId;
        if (paramGrantType != "authorization_code" || paramCode.length() == 0 || paramRedirectUri.length() == 0 ||
            paramClientId.length() == 0) {
            VLOG(0) << paramGrantType << ", " << paramCode << ", " << paramRedirectUri << ", " << paramClientId;
            VLOG(0) << "Token request invalid, sending Bad Request";
            res.sendStatus(400);
            return;
        }
        // UUID example: 4a3797ae-0734-4f7f-ad2c-1c1bfafba7da
        // cat /proc/sys/kernel/random/uuid
        // Open file -> readline -> close
        res.send("");
    });

    app.use(express::middleware::StaticMiddleware("/home/rathalin/projects/snode.c/src/oauth2/authorization_server/"
                                                  "vue-frontend/dist"));

    app.listen(8082, [](const express::legacy::in::WebApp::SocketAddress socketAddress, int err) {
        if (err != 0) {
            std::cerr << "Failed to listen on port " << 8082 << std::endl;
        } else {
            std::cout << "OAuth2AuthorizationServer is listening on " << socketAddress.toString() << std::endl;
        }
    });

    return express::WebApp::start();
}
