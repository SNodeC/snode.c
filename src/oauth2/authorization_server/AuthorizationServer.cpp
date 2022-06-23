#include "database/mariadb/MariaDBClient.h"
#include "database/mariadb/MariaDBCommandSequence.h"
#include "express/Router.h"
#include "express/legacy/in/WebApp.h"
#include "express/middleware/JsonMiddleware.h"
#include "express/middleware/StaticMiddleware.h"
#include "express/middleware/VHost.h"
#include "utils/sha1.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

void addQueryParamToUri(std::string& uri, std::string queryParamName, std::string queryParamValue) {
    if (uri.find("?") == std::string::npos) {
        uri += "?";
    } else {
        uri += "&";
    }
    uri += queryParamName + "=" + queryParamValue;
}

std::string timeToString(std::chrono::time_point<std::chrono::system_clock> time) {
    auto in_time_t = std::chrono::system_clock::to_time_t(time);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}

std::string getNewUUID() {
    const size_t uuidLength{36};
    char uuidCharArray[uuidLength];
    std::ifstream file("/proc/sys/kernel/random/uuid");
    file.getline(uuidCharArray, uuidLength);
    file.close();
    return std::string{uuidCharArray};
}

std::string hashSha1(const std::string& str) {
    SHA1 checksum;
    checksum.update(str);
    return checksum.final();
}

/*
void validClientId(express::Request& req, express::Response& res, database::mariadb::MariaDBClient& db, std::function<void()> onValid) {
    std::string queryClientId{req.query("client_id")};
    if (queryClientId.length() > 0) {
        db.query(
            "select count(*) from client where uuid = '" + queryClientId + "'",
            [&res, queryClientId, onValid](const MYSQL_ROW row) -> void {
                if (row != nullptr) {
                    if (std::stoi(row[0]) > 0) {
                        VLOG(0) << "Valid client id '" << queryClientId << "'";
                        onValid();
                    } else {
                        VLOG(0) << "Invalid client id '" << queryClientId << "'";
                        res.status(401).send("Invalid query parameter 'client_id' with value '" + queryClientId + "'");
                    }
                }
            },
            [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                res.sendStatus(500);
            });
    } else {
        res.status(401).send("Missing query parameter 'client_id'");
    }
}
*/

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

    express::Router router{};

    // Middleware to catch requests without a valid client_id
    router.use([&db] MIDDLEWARE(req, res, next) {
        std::string queryClientId{req.query("client_id")};
        if (queryClientId.length() > 0) {
            db.query(
                "select count(*) from client where uuid = '" + queryClientId + "'",
                [&req, &res, next, queryClientId](const MYSQL_ROW row) -> void {
                    if (row != nullptr) {
                        if (std::stoi(row[0]) > 0) {
                            VLOG(0) << "Valid client id '" << queryClientId << "'";
                            VLOG(0) << "Next with " << req.httpVersion << " " << req.method << " " << req.url;
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
            res.status(401).send("Invalid client_id");
        }
    });

    router.get("/authorize", [&db] APPLICATION(req, res) {
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
        std::string loginUri{"/oauth2/login"};
        addQueryParamToUri(loginUri, "client_id", paramClientId);
        res.redirect(loginUri);
    });

    router.get("/login", [&db] APPLICATION(req, res) {
        res.sendFile("/home/rathalin/projects/snode.c/src/oauth2/authorization_server/vue-frontend-oauth2-auth-server/dist/index.html",
                     [&req](int ret) -> void {
                         if (ret != 0) {
                             PLOG(ERROR) << req.url;
                         }
                     });
    });

    router.post("/login", [&db] APPLICATION(req, res) {
        req.getAttribute<nlohmann::json>(
            [&req, &res, &db](nlohmann::json& body) -> void {
                db.query(
                    "select email, password_hash, password_salt, redirect_uri, state "
                    "from client "
                    "where uuid = '" +
                        req.query("client_id") + "'",
                    [&req, &res, &db, &body](const MYSQL_ROW row) -> void {
                        if (row != nullptr) {
                            std::string dbEmail{row[0]};
                            std::string dbPasswordHash{row[1]};
                            std::string dbPasswordSalt{row[2]};
                            std::string dbRedirectUri{row[3]};
                            std::string dbState{row[4]};
                            std::string queryEmail{body["email"]};
                            std::string queryPassword{body["password"]};
                            // Check email and password
                            if (dbEmail != queryEmail) {
                                res.status(401).send("Invalid email address");
                            } else if (dbPasswordHash != hashSha1(dbPasswordSalt + queryPassword)) {
                                res.status(401).send("Invalid password");
                            } else {
                                // Generate auth code which expires after 10 minutes
                                unsigned int expireMinutes{10};
                                std::string authCode{getNewUUID()};
                                db.exec(
                                      "insert into token(uuid, expire_datetime) "
                                      "values('" +
                                          authCode + "', '" +
                                          timeToString(std::chrono::system_clock::now() + std::chrono::minutes(expireMinutes)) + "')",
                                      []() -> void {
                                      },
                                      [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                                          VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                                          res.sendStatus(500);
                                      })
                                    .query(
                                        "select last_insert_id()",
                                        [&req, &res, &db, dbState, dbRedirectUri, authCode](const MYSQL_ROW row) -> void {
                                            if (row != nullptr) {
                                                db.exec(
                                                    "update client "
                                                    "set auth_code_id = '" +
                                                        std::string{row[0]} +
                                                        "' "
                                                        "where uuid = '" +
                                                        req.query("client_id") + "'",
                                                    [&res, dbState, dbRedirectUri, authCode]() -> void {
                                                        // Redirect back to the client app
                                                        std::string clientRedirectUri{dbRedirectUri};
                                                        addQueryParamToUri(clientRedirectUri, "code", authCode);
                                                        if (!dbState.empty()) {
                                                            addQueryParamToUri(clientRedirectUri, "state", dbState);
                                                        }
                                                        // Set CORS header
                                                        res.set("Access-Control-Allow-Origin", "*");
                                                        nlohmann::json responseJson = {{"redirect_uri", clientRedirectUri}};
                                                        std::string responseJsonString{responseJson.dump(4)};
                                                        VLOG(0) << "Sending json reponse: " << responseJsonString;
                                                        res.send(responseJsonString);
                                                    },
                                                    [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                                                        VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                                                        res.sendStatus(500);
                                                    });
                                            }
                                        },
                                        [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                                            VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                                            res.sendStatus(500);
                                        });
                            }
                        }
                    },
                    [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                        VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                        res.sendStatus(500);
                    });
            },
            [&res]([[maybe_unused]] const std::string& key) -> void {
                res.sendStatus(500);
            });
    });

    router.get("/token", [&db] APPLICATION(req, res) {
        res.set("Access-Control-Allow-Origin", "*");
        auto queryGrantType = req.query("grant_type");
        VLOG(0) << "GrandType: " << queryGrantType;
        auto queryCode = req.query("code");
        VLOG(0) << "Code: " << queryCode;
        auto queryRedirectUri = req.query("redirect_uri");
        VLOG(0) << "RedirectUri: " << queryRedirectUri;
        if (queryGrantType != "authorization_code") {
            res.status(400).send("Invalid query parameter 'grant_type', value must be 'authorization_code'");
            return;
        }
        if (queryCode.length() == 0) {
            res.status(400).send("Missing query parameter 'code'");
            return;
        }
        if (queryRedirectUri.length() == 0) {
            res.status(400).send("Missing query parameter 'redirect_uri'");
            return;
        }
        db.query(
            "select count(*) "
            "from client "
            "where uuid = '" +
                req.query("client_id") +
                "' "
                "and redirect_uri = '" +
                queryRedirectUri + "'",
            [&req, &res, &db](const MYSQL_ROW row) -> void {
                if (row != nullptr) {
                    if (std::stoi(row[0]) == 0) {
                        res.status(400).send("Query param 'redirect_uri' must be the same as in the initial request");
                    } else {
                        db.query(
                            "select count(*) "
                            "from client c "
                            "join token a "
                            "on c.auth_code_id = a.id "
                            "where c.uuid = '" +
                                req.query("client_id") +
                                "' "
                                "and a.uuid = '" +
                                req.query("code") +
                                "' "
                                "and timestampdiff(second, current_timestamp(), a.expire_datetime) > 0",
                            [&req, &res, &db](const MYSQL_ROW row) -> void {
                                if (row != nullptr) {
                                    if (std::stoi(row[0]) == 0) {
                                        res.status(401).send("Invalid auth token");
                                        return;
                                    }
                                    // Generate access and refresh token
                                    std::string accessToken{getNewUUID()};
                                    unsigned int accessTokenExpireSeconds{60 * 60}; // 1 hour
                                    std::string refreshToken{getNewUUID()};
                                    unsigned int refreshTokenExpireSeconds{60 * 60 * 24}; // 24 hours
                                    db.exec(
                                          "insert into token(uuid, expire_datetime) "
                                          "values('" +
                                              accessToken + "', '" +
                                              timeToString(std::chrono::system_clock::now() +
                                                           std::chrono::seconds(accessTokenExpireSeconds)) +
                                              "')",
                                          []() -> void {
                                          },
                                          [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                                              VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                                              res.sendStatus(500);
                                          })
                                        .query(
                                            "select last_insert_id()",
                                            [&req, &res, &db](const MYSQL_ROW row) -> void {
                                                if (row != nullptr) {
                                                    db.exec(
                                                        "update client "
                                                        "set access_token_id = '" +
                                                            std::string{row[0]} +
                                                            "' "
                                                            "where uuid = '" +
                                                            req.query("client_id") + "'",
                                                        []() -> void {
                                                        },
                                                        [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                                                            VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                                                            res.sendStatus(500);
                                                        });
                                                }
                                            },
                                            [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                                                VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                                                res.sendStatus(500);
                                            })
                                        .exec(
                                            "insert into token(uuid, expire_datetime) "
                                            "values('" +
                                                refreshToken + "', '" +
                                                timeToString(std::chrono::system_clock::now() +
                                                             std::chrono::seconds(refreshTokenExpireSeconds)) +
                                                "')",
                                            []() -> void {
                                            },
                                            [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                                                VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                                                res.sendStatus(500);
                                            })
                                        .query(
                                            "select last_insert_id()",
                                            [&req, &res, &db, accessToken, accessTokenExpireSeconds, refreshToken](
                                                const MYSQL_ROW row) -> void {
                                                if (row != nullptr) {
                                                    db.exec(
                                                        "update client "
                                                        "set refresh_token_id = '" +
                                                            std::string{row[0]} +
                                                            "' "
                                                            "where uuid = '" +
                                                            req.query("client_id") + "'",
                                                        [&res, accessToken, accessTokenExpireSeconds, refreshToken]() -> void {
                                                            // Send auth token and refresh token
                                                            nlohmann::json jsonResponse = {{"access_token", accessToken},
                                                                                           {"expires_in", accessTokenExpireSeconds},
                                                                                           {"refresh_token", refreshToken}};
                                                            std::string jsonResponseString{jsonResponse.dump(4)};
                                                            res.send(jsonResponseString);
                                                        },
                                                        [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                                                            VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                                                            res.sendStatus(500);
                                                        });
                                                }
                                            },
                                            [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                                                VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                                                res.sendStatus(500);
                                            });
                                }
                            },
                            [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                                VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                                res.sendStatus(500);
                            });
                    }
                }
            },
            [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                res.sendStatus(500);
            });
    });

    router.post("/token/refresh", [&db] APPLICATION(req, res) {
        res.set("Access-Control-Allow-Origin", "*");
        auto queryClientId = req.query("client_id");
        VLOG(0) << "ClientId: " << queryClientId;
        auto queryGrantType = req.query("grant_type");
        VLOG(0) << "GrandType: " << queryGrantType;
        auto queryRefreshToken = req.query("refresh_token");
        VLOG(0) << "RefreshToken: " << queryRefreshToken;
        auto queryState = req.query("state");
        VLOG(0) << "State: " << queryState;
        if (queryGrantType.length() == 0) {
            res.status(400).send("Missing query parameter 'grant_type'");
            return;
        }
        if (queryGrantType != "refresh_token") {
            res.status(400).send("Invalid query parameter 'grant_type', value must be 'refresh_token'");
            return;
        }
        if (queryRefreshToken.empty()) {
            res.status(400).send("Missing query parameter 'refresh_token'");
        }
        db.query(
            "select count(*) "
            "from client c "
            "join token r "
            "on c.refresh_token_id = r.id "
            "where c.uuid = '" +
                req.query("client_id") +
                "' "
                "and r.uuid = '" +
                req.query("refresh_token") +
                "' "
                "and timestampdiff(second, current_timestamp(), r.expire_datetime) > 0",
            [&req, &res, &db](const MYSQL_ROW row) -> void {
                if (row != nullptr) {
                    if (std::stoi(row[0]) == 0) {
                        res.status(401).send("Invalid refresh token");
                        return;
                    }
                    // Generate access token
                    std::string accessToken{getNewUUID()};
                    unsigned int accessTokenExpireSeconds{60 * 60}; // 1 hour
                    db.exec(
                          "insert into token(uuid, expire_datetime) "
                          "values('" +
                              accessToken + "', '" +
                              timeToString(std::chrono::system_clock::now() + std::chrono::seconds(accessTokenExpireSeconds)) + "')",
                          []() -> void {
                          },
                          [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                              VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                              res.sendStatus(500);
                          })
                        .query(
                            "select last_insert_id()",
                            [&req, &res, &db, accessToken](const MYSQL_ROW row) -> void {
                                if (row != nullptr) {
                                    db.exec(
                                        "update client "
                                        "set access_token_id = '" +
                                            std::string{row[0]} +
                                            "' "
                                            "where uuid = '" +
                                            req.query("client_id") + "'",
                                        [&res, accessToken]() -> void {
                                            nlohmann::json responseJson = {{"access_token", accessToken}};
                                            res.send(responseJson.dump(4));
                                        },
                                        [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                                            VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                                            res.sendStatus(500);
                                        });
                                }
                            },
                            [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                                VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                                res.sendStatus(500);
                            });
                }
            },
            [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                res.sendStatus(500);
            });
    });

    router.post("/token/validate", [&db] APPLICATION(req, res) {
        VLOG(0) << "POST /token/validate";
        req.getAttribute<nlohmann::json>([&req, &res, &db](nlohmann::json& jsonBody) -> void {
            if (!jsonBody.contains("access_token")) {
                VLOG(0) << "Missing 'access_token' in json";
                res.status(500).send("Missing 'access_token' in json");
                return;
            }
            std::string jsonAccessToken{jsonBody["access_token"]};
            if (!jsonBody.contains("client_id")) {
                VLOG(0) << "Missing 'client_id' in json";
                res.status(500).send("Missing 'client_id' in json");
                return;
            }
            std::string jsonClientId{jsonBody["client_id"]};
            db.query(
                "select count(*) "
                "from client c "
                "join token a "
                "on c.access_token_id = a.id "
                "where c.uuid = '" +
                    jsonClientId +
                    "' "
                    "and a.uuid = '" +
                    jsonAccessToken + "'",
                [&res, jsonClientId, jsonAccessToken](const MYSQL_ROW row) -> void {
                    if (row != nullptr) {
                        if (std::stoi(row[0]) == 0) {
                            nlohmann::json errorJson = {{"error", "Invalid access token"}};
                            VLOG(0) << "########################################" << jsonClientId + ", " + jsonAccessToken;
                            res.status(401).send(errorJson.dump(4));
                        } else {
                            VLOG(0) << "######################################### valid access token";
                            nlohmann::json successJson = {{"success", "Valid access token"}};
                            res.status(200).send(successJson.dump(4));
                        }
                    }
                },
                [&res](const std::string& errorString, unsigned int errorNumber) -> void {
                    VLOG(0) << "Database error: " << errorString << " : " << errorNumber;
                    res.sendStatus(500);
                });
        });
    });

    app.use("/oauth2", router);
    app.use(express::middleware::StaticMiddleware(
        "/home/rathalin/projects/snode.c/src/oauth2/authorization_server/vue-frontend-oauth2-auth-server/dist/"));

    app.listen(8082, [](const express::legacy::in::WebApp::SocketAddress socketAddress, int err) {
        if (err != 0) {
            std::cerr << "Failed to listen on port " << 8082 << std::endl;
        } else {
            std::cout << "OAuth2AuthorizationServer is listening on " << socketAddress.toString() << std::endl;
        }
    });

    return express::WebApp::start();
}
