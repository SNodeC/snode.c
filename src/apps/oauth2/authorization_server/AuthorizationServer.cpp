/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "database/mariadb/MariaDBClient.h"
#include "database/mariadb/MariaDBCommandSequence.h"
#include "express/legacy/in/WebApp.h"
#include "express/middleware/JsonMiddleware.h"
#include "express/middleware/StaticMiddleware.h"
#include "log/Logger.h"
#include "utils/sha1.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <mysql.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>

// IWYU pragma: no_include <nlohmann/json_fwd.hpp>
// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>

void addQueryParamToUri(std::string& uri, const std::string& queryParamName, const std::string& queryParamValue) {
    if (uri.find('?') == std::string::npos) {
        uri += '?';
    } else {
        uri += '&';
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
    utils::SHA1 checksum;
    checksum.update(str);
    return checksum.final();
}

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);
    const express::legacy::in::WebApp app("OAuth2AuthorizationServer");

    const database::mariadb::MariaDBConnectionDetails details{
        .hostname = "localhost",
        .username = "rathalin",
        .password = "rathalin",
        .database = "oauth2",
        .port = 3306,
        .socket = "/run/mysqld/mysqld.sock",
        .flags = 0,
    };
    database::mariadb::MariaDBClient db{details, [](const database::mariadb::MariaDBState& state) {
                                            if (state.error != 0) {
                                                VLOG(0) << "MySQL error: " << state.errorMessage << " [" << state.error << "]";
                                            } else if (state.connected) {
                                                VLOG(0) << "MySQL connected";
                                            } else {
                                                VLOG(0) << "MySQL disconnected";
                                            }
                                        }};

    app.use(express::middleware::JsonMiddleware());

    const express::Router router{};

    // Middleware to catch requests without a valid client_id
    router.use([&db] MIDDLEWARE(req, res, next) {
        const std::string queryClientId{req->query("client_id")};
        if (queryClientId.length() > 0) {
            db.query(
                "select count(*) from client where uuid = '" + queryClientId + "'",
                [req, res, next, queryClientId](const MYSQL_ROW row) {
                    if (row != nullptr) {
                        if (std::stoi(row[0]) > 0) {
                            VLOG(1) << "Valid client id '" << queryClientId << "'";
                            VLOG(1) << "Next with " << req->httpVersion << " " << req->method << " " << req->url;
                            next();
                        } else {
                            VLOG(1) << "Invalid client id '" << queryClientId << "'";
                            res->sendStatus(401);
                        }
                    }
                },
                [res](const std::string& errorString, unsigned int errorNumber) {
                    VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                    res->sendStatus(500);
                });
        } else {
            res->status(401).send("Invalid client_id");
        }
    });

    router.get("/authorize", [&db] APPLICATION(req, res) {
        // REQUIRED: response_type, client_id
        // OPTIONAL: redirect_uri, scope
        // RECOMMENDED: state
        const std::string paramResponseType{req->query("response_type")};
        const std::string paramClientId{req->query("client_id")};
        const std::string paramRedirectUri{req->query("redirect_uri")};
        const std::string paramScope{req->query("scope")};
        const std::string paramState{req->query("state")};

        VLOG(1) << "Query params: "
                << "response_type=" << req->query("response_type") << ", "
                << "redirect_uri=" << req->query("redirect_uri") << ", "
                << "scope=" << req->query("scope") << ", "
                << "state=" << req->query("state") << "\n";

        if (paramResponseType != "code") {
            VLOG(1) << "Auth invalid, sending Bad Request";
            res->sendStatus(400);
            return;
        }

        if (!paramRedirectUri.empty()) {
            db.exec(
                "update client set redirect_uri = '" + paramRedirectUri + "' where uuid = '" + paramClientId + "'",
                [paramRedirectUri]() {
                    VLOG(1) << "Database: Set redirect_uri to " << paramRedirectUri;
                },
                [](const std::string& errorString, unsigned int errorNumber) {
                    VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                });
        }

        if (!paramScope.empty()) {
            db.exec(
                "update client set scope = '" + paramScope + "' where uuid = '" + paramClientId + "'",
                [paramScope]() {
                    VLOG(1) << "Database: Set scope to " << paramScope;
                },
                [](const std::string& errorString, unsigned int errorNumber) {
                    VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                });
        }

        if (!paramState.empty()) {
            db.exec(
                "update client set state = '" + paramState + "' where uuid = '" + paramClientId + "'",
                [paramState]() {
                    VLOG(1) << "Database: Set state to " << paramState;
                },
                [](const std::string& errorString, unsigned int errorNumber) {
                    VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                });
        }

        VLOG(1) << "Auth request valid, redirecting to login";
        std::string loginUri{"/oauth2/login"};
        addQueryParamToUri(loginUri, "client_id", paramClientId);
        res->redirect(loginUri);
    });

    router.get("/login", [] APPLICATION(req, res) {
        res->sendFile("/home/rathalin/projects/snode.c/src/oauth2/authorization_server/vue-frontend-oauth2-auth-server/dist/index.html",
                      [req](int ret) {
                          if (ret != 0) {
                              PLOG(ERROR) << req->url;
                          }
                      });
    });

    router.post("/login", [&db] APPLICATION(req, res) {
        req->getAttribute<nlohmann::json>(
            [req, res, &db](nlohmann::json& body) {
                db.query(
                    "select email, password_hash, password_salt, redirect_uri, state "
                    "from client "
                    "where uuid = '" +
                        req->query("client_id") + "'",
                    [req, res, &db, &body](const MYSQL_ROW row) {
                        if (row != nullptr) {
                            const std::string dbEmail{row[0]};
                            const std::string dbPasswordHash{row[1]};
                            const std::string dbPasswordSalt{row[2]};
                            const std::string dbRedirectUri{row[3]};
                            const std::string dbState{row[4]};
                            const std::string queryEmail{body["email"]};
                            const std::string queryPassword{body["password"]};
                            // Check email and password
                            if (dbEmail != queryEmail) {
                                res->status(401).send("Invalid email address");
                            } else if (dbPasswordHash != hashSha1(dbPasswordSalt + queryPassword)) {
                                res->status(401).send("Invalid password");
                            } else {
                                // Generate auth code which expires after 10 minutes
                                const unsigned int expireMinutes{10};
                                const std::string authCode{getNewUUID()};
                                db.exec(
                                      "insert into token(uuid, expire_datetime) "
                                      "values('" +
                                          authCode + "', '" +
                                          timeToString(std::chrono::system_clock::now() + std::chrono::minutes(expireMinutes)) + "')",
                                      []() {
                                      },
                                      [res](const std::string& errorString, unsigned int errorNumber) {
                                          VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                                          res->sendStatus(500);
                                      })
                                    .query(
                                        "select last_insert_id()",
                                        [req, res, &db, dbState, dbRedirectUri, authCode](const MYSQL_ROW row) {
                                            if (row != nullptr) {
                                                db.exec(
                                                    "update client "
                                                    "set auth_code_id = '" +
                                                        std::string{row[0]} +
                                                        "' "
                                                        "where uuid = '" +
                                                        req->query("client_id") + "'",
                                                    [res, dbState, dbRedirectUri, authCode]() {
                                                        // Redirect back to the client app
                                                        std::string clientRedirectUri{dbRedirectUri};
                                                        addQueryParamToUri(clientRedirectUri, "code", authCode);
                                                        if (!dbState.empty()) {
                                                            addQueryParamToUri(clientRedirectUri, "state", dbState);
                                                        }
                                                        // Set CORS header
                                                        res->set("Access-Control-Allow-Origin", "*");
                                                        const nlohmann::json responseJson = {{"redirect_uri", clientRedirectUri}};
                                                        const std::string responseJsonString{responseJson.dump(4)};
                                                        VLOG(1) << "Sending json reponse: " << responseJsonString;
                                                        res->send(responseJsonString);
                                                    },
                                                    [res](const std::string& errorString, unsigned int errorNumber) {
                                                        VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                                                        res->sendStatus(500);
                                                    });
                                            }
                                        },
                                        [res](const std::string& errorString, unsigned int errorNumber) {
                                            VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                                            res->sendStatus(500);
                                        });
                            }
                        }
                    },
                    [res](const std::string& errorString, unsigned int errorNumber) {
                        VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                        res->sendStatus(500);
                    });
            },
            [res]([[maybe_unused]] const std::string& key) {
                res->sendStatus(500);
            });
    });

    router.get("/token", [&db] APPLICATION(req, res) {
        res->set("Access-Control-Allow-Origin", "*");
        auto queryGrantType = req->query("grant_type");
        VLOG(1) << "GrandType: " << queryGrantType;
        auto queryCode = req->query("code");
        VLOG(1) << "Code: " << queryCode;
        auto queryRedirectUri = req->query("redirect_uri");
        VLOG(1) << "RedirectUri: " << queryRedirectUri;
        if (queryGrantType != "authorization_code") {
            res->status(400).send("Invalid query parameter 'grant_type', value must be 'authorization_code'");
            return;
        }
        if (queryCode.length() == 0) {
            res->status(400).send("Missing query parameter 'code'");
            return;
        }
        if (queryRedirectUri.length() == 0) {
            res->status(400).send("Missing query parameter 'redirect_uri'");
            return;
        }
        db.query(
            "select count(*) "
            "from client "
            "where uuid = '" +
                req->query("client_id") +
                "' "
                "and redirect_uri = '" +
                queryRedirectUri + "'",
            [req, res, &db](const MYSQL_ROW row) {
                if (row != nullptr) {
                    if (std::stoi(row[0]) == 0) {
                        res->status(400).send("Query param 'redirect_uri' must be the same as in the initial request");
                    } else {
                        db.query(
                            "select count(*) "
                            "from client c "
                            "join token a "
                            "on c.auth_code_id = a.id "
                            "where c.uuid = '" +
                                req->query("client_id") +
                                "' "
                                "and a.uuid = '" +
                                req->query("code") +
                                "' "
                                "and timestampdiff(second, current_timestamp(), a.expire_datetime) > 0",
                            [req, res, &db](const MYSQL_ROW row) {
                                if (row != nullptr) {
                                    if (std::stoi(row[0]) == 0) {
                                        res->status(401).send("Invalid auth token");
                                        return;
                                    }
                                    // Generate access and refresh token
                                    const std::string accessToken{getNewUUID()};
                                    const unsigned int accessTokenExpireSeconds{60 * 60}; // 1 hour
                                    const std::string refreshToken{getNewUUID()};
                                    const unsigned int refreshTokenExpireSeconds{60 * 60 * 24}; // 24 hours
                                    db.exec(
                                          "insert into token(uuid, expire_datetime) "
                                          "values('" +
                                              accessToken + "', '" +
                                              timeToString(std::chrono::system_clock::now() +
                                                           std::chrono::seconds(accessTokenExpireSeconds)) +
                                              "')",
                                          []() {
                                          },
                                          [res](const std::string& errorString, unsigned int errorNumber) {
                                              VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                                              res->sendStatus(500);
                                          })
                                        .query(
                                            "select last_insert_id()",
                                            [req, res, &db](const MYSQL_ROW row) {
                                                if (row != nullptr) {
                                                    db.exec(
                                                        "update client "
                                                        "set access_token_id = '" +
                                                            std::string{row[0]} +
                                                            "' "
                                                            "where uuid = '" +
                                                            req->query("client_id") + "'",
                                                        []() {
                                                        },
                                                        [res](const std::string& errorString, unsigned int errorNumber) {
                                                            VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                                                            res->sendStatus(500);
                                                        });
                                                }
                                            },
                                            [res](const std::string& errorString, unsigned int errorNumber) {
                                                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                                                res->sendStatus(500);
                                            })
                                        .exec(
                                            "insert into token(uuid, expire_datetime) "
                                            "values('" +
                                                refreshToken + "', '" +
                                                timeToString(std::chrono::system_clock::now() +
                                                             std::chrono::seconds(refreshTokenExpireSeconds)) +
                                                "')",
                                            []() {
                                            },
                                            [res](const std::string& errorString, unsigned int errorNumber) {
                                                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                                                res->sendStatus(500);
                                            })
                                        .query(
                                            "select last_insert_id()",
                                            [req, res, &db, accessToken, accessTokenExpireSeconds, refreshToken](const MYSQL_ROW row) {
                                                if (row != nullptr) {
                                                    db.exec(
                                                        "update client "
                                                        "set refresh_token_id = '" +
                                                            std::string{row[0]} +
                                                            "' "
                                                            "where uuid = '" +
                                                            req->query("client_id") + "'",
                                                        [res, accessToken, accessTokenExpireSeconds, refreshToken]() {
                                                            // Send auth token and refresh token
                                                            const nlohmann::json jsonResponse = {{"access_token", accessToken},
                                                                                                 {"expires_in", accessTokenExpireSeconds},
                                                                                                 {"refresh_token", refreshToken}};
                                                            const std::string jsonResponseString{jsonResponse.dump(4)};
                                                            res->send(jsonResponseString);
                                                        },
                                                        [res](const std::string& errorString, unsigned int errorNumber) {
                                                            VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                                                            res->sendStatus(500);
                                                        });
                                                }
                                            },
                                            [res](const std::string& errorString, unsigned int errorNumber) {
                                                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                                                res->sendStatus(500);
                                            });
                                }
                            },
                            [res](const std::string& errorString, unsigned int errorNumber) {
                                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                                res->sendStatus(500);
                            });
                    }
                }
            },
            [res](const std::string& errorString, unsigned int errorNumber) {
                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                res->sendStatus(500);
            });
    });

    router.post("/token/refresh", [&db] APPLICATION(req, res) {
        res->set("Access-Control-Allow-Origin", "*");
        auto queryClientId = req->query("client_id");
        VLOG(1) << "ClientId: " << queryClientId;
        auto queryGrantType = req->query("grant_type");
        VLOG(1) << "GrandType: " << queryGrantType;
        auto queryRefreshToken = req->query("refresh_token");
        VLOG(1) << "RefreshToken: " << queryRefreshToken;
        auto queryState = req->query("state");
        VLOG(1) << "State: " << queryState;
        if (queryGrantType.length() == 0) {
            res->status(400).send("Missing query parameter 'grant_type'");
            return;
        }
        if (queryGrantType != "refresh_token") {
            res->status(400).send("Invalid query parameter 'grant_type', value must be 'refresh_token'");
            return;
        }
        if (queryRefreshToken.empty()) {
            res->status(400).send("Missing query parameter 'refresh_token'");
        }
        db.query(
            "select count(*) "
            "from client c "
            "join token r "
            "on c.refresh_token_id = r.id "
            "where c.uuid = '" +
                req->query("client_id") +
                "' "
                "and r.uuid = '" +
                req->query("refresh_token") +
                "' "
                "and timestampdiff(second, current_timestamp(), r.expire_datetime) > 0",
            [req, res, &db](const MYSQL_ROW row) {
                if (row != nullptr) {
                    if (std::stoi(row[0]) == 0) {
                        res->status(401).send("Invalid refresh token");
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
                          []() {
                          },
                          [res](const std::string& errorString, unsigned int errorNumber) {
                              VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                              res->sendStatus(500);
                          })
                        .query(
                            "select last_insert_id()",
                            [req, res, &db, accessToken, accessTokenExpireSeconds](const MYSQL_ROW row) {
                                if (row != nullptr) {
                                    db.exec(
                                        "update client "
                                        "set access_token_id = '" +
                                            std::string{row[0]} +
                                            "' "
                                            "where uuid = '" +
                                            req->query("client_id") + "'",
                                        [res, accessToken, accessTokenExpireSeconds]() {
                                            const nlohmann::json responseJson = {{"access_token", accessToken},
                                                                                 {"expires_in", accessTokenExpireSeconds}};
                                            res->send(responseJson.dump(4));
                                        },
                                        [res](const std::string& errorString, unsigned int errorNumber) {
                                            VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                                            res->sendStatus(500);
                                        });
                                }
                            },
                            [res](const std::string& errorString, unsigned int errorNumber) {
                                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                                res->sendStatus(500);
                            });
                }
            },
            [res](const std::string& errorString, unsigned int errorNumber) {
                VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                res->sendStatus(500);
            });
    });

    router.post("/token/validate", [&db] APPLICATION(req, res) {
        VLOG(1) << "POST /token/validate";
        req->getAttribute<nlohmann::json>([res, &db](nlohmann::json& jsonBody) {
            if (!jsonBody.contains("access_token")) {
                VLOG(1) << "Missing 'access_token' in json";
                res->status(500).send("Missing 'access_token' in json");
                return;
            }
            const std::string jsonAccessToken{jsonBody["access_token"]};
            if (!jsonBody.contains("client_id")) {
                VLOG(1) << "Missing 'client_id' in json";
                res->status(500).send("Missing 'client_id' in json");
                return;
            }
            const std::string jsonClientId{jsonBody["client_id"]};
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
                [res, jsonClientId, jsonAccessToken](const MYSQL_ROW row) {
                    if (row != nullptr) {
                        if (std::stoi(row[0]) == 0) {
                            const nlohmann::json errorJson = {{"error", "Invalid access token"}};
                            VLOG(1) << "Sending 401: Invalid access token '" << jsonAccessToken << "'";
                            res->status(401).send(errorJson.dump(4));
                        } else {
                            VLOG(1) << "Sending 200: Valid access token '" << jsonAccessToken << "";
                            const nlohmann::json successJson = {{"success", "Valid access token"}};
                            res->status(200).send(successJson.dump(4));
                        }
                    }
                },
                [res](const std::string& errorString, unsigned int errorNumber) {
                    VLOG(1) << "Database error: " << errorString << " : " << errorNumber;
                    res->sendStatus(500);
                });
        });
    });

    app.use("/oauth2", router);
    app.use(express::middleware::StaticMiddleware(
        "/home/rathalin/projects/snode.c/src/oauth2/authorization_server/vue-frontend-oauth2-auth-server/dist/"));

    app.listen(8082, [](const express::legacy::in::WebApp::SocketAddress& socketAddress, const core::socket::State& state) {
        switch (state) {
            case core::socket::State::OK:
                VLOG(1) << "OAuth2AuthorizationServer: listening on '" << socketAddress.toString() << "'";
                break;
            case core::socket::State::DISABLED:
                VLOG(1) << "OAuth2AuthorizationServer: disabled";
                break;
            case core::socket::State::ERROR:
                VLOG(1) << "OAuth2AuthorizationServer: error occurred";
                break;
            case core::socket::State::FATAL:
                VLOG(1) << "OAuth2AuthorizationServer: fatal error occurred";
                break;
        }
    });

    return express::WebApp::start();
}
