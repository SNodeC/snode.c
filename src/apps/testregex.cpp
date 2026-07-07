/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include "SemanticLog.h"
#include "database/mariadb/MariaDBClient.h"
#include "express/legacy/in/WebApp.h"
#include "express/tls/in/WebApp.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstddef>
#include <iostream>
#include <mysql.h>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

// IWYU pragma: no_include <openssl/ssl3.h>
// IWYU pragma: no_include <openssl/x509.h>
// IWYU pragma: no_include <openssl/types.h>
// IWYU pragma: no_include <openssl/asn1.h>
// IWYU pragma: no_include <openssl/obj_mac.h>
// IWYU pragma: no_include <openssl/crypto.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

Router router(database::mariadb::MariaDBClient& db) {
    const Router router;

    router.get(R"(/r1/:id([^/]+))", [] APPLICATION(req, res) {
        std::cout << "Hier 1a ************" << std::endl;
        std::cout << "Params: " << req->params["id"] << std::endl;
        res->send("Done\n");
    });

    router.get(R"(/r1/:id/:subcollection)", [] APPLICATION(req, res) {
        std::cout << "Hier 1b ************" << std::endl;
        std::cout << "Params: " << req->params["id"] << std::endl;
        std::cout << "Params: " << req->params["subcollection"] << std::endl;
        res->send("Done\n");
    });

    router.get(R"(/r2/:id)", [] APPLICATION(req, res) {
        std::cout << "Hier 2a ************" << std::endl;
        std::cout << "Params: " << req->params["id"] << std::endl;
        res->send("Done\n");
    });

    router.get(R"(/r2/:id/:subcollection)", [] APPLICATION(req, res) {
        std::cout << "Hier 2b ************" << std::endl;
        std::cout << "Params: " << req->params["id"] << std::endl;
        std::cout << "Params: " << req->params["subcollection"] << std::endl;
        res->send("Done\n");
    });

    router.get("/test/:variable(\\d)/:uri", [] APPLICATION(req, res) { // http://localhost:8080/test/1/urlstring
        std::cout << "Hier 3 ************" << std::endl;
        std::cout << "Params: " << req->params["variable"] << std::endl;
        std::cout << "Params: " << req->params["uri"] << std::endl;
        res->send("Done\n");
    });

    router
        .get(
            "/query/:userId",
            [] MIDDLEWARE(req, res, next) {
                snode::semantic::appLog().debug() << "Move on to the next route to query database";
                next();
            },
            [&db] MIDDLEWARE(req, res, next) { // http://localhost:8080/query/123
                snode::semantic::appLog().debug() << "UserId: " << req->params["userId"];
                std::string userId = req->params["userId"];

                req->setAttribute<std::string, "html-table">(std::string());

                req->getAttribute<std::string, "html-table">([&userId](std::string& table) {
                    table = "<html>\n"
                            "  <head>\n"
                            "    <title>"
                            "Response from snode.c for " +
                            userId +
                            "\n"
                            "    </title>\n"
                            "  </head>\n"
                            "  <body>\n"
                            "    <h1>Return for " +
                            userId +
                            "\n"
                            "    </h1>\n"
                            "  <body>\n"
                            "    <table border = \"1\">\n";
                });

                int i = 0;
                db.query(
                    "SELECT * FROM snodec where username = '" + userId + "'",
                    [next, req, i](const MYSQL_ROW row) mutable {
                        if (row != nullptr) {
                            i++;
                            req->getAttribute<std::string, "html-table">([row, &i](std::string& table) {
                                table.append("      <tr>\n"
                                             "        <td>\n" +
                                             std::to_string(i) +
                                             "\n"
                                             "        </td>\n"
                                             "        <td>\n" +
                                             std::string(row[0]) +
                                             "\n"
                                             "        </td>\n"
                                             "        <td>\n" +
                                             row[1] +
                                             "\n"
                                             "        </td>\n"
                                             "      </tr>\n");
                            });
                        } else {
                            req->getAttribute<std::string, "html-table">([](std::string& table) {
                                table.append(std::string("    </table>\n"
                                                         "  </body>\n"
                                                         "</html>\n"));
                            });
                            snode::semantic::appLog().debug() << "Move on to the next route to send result";
                            next();
                        }
                    },
                    [res, userId](const std::string& errorString, unsigned int errorNumber) {
                        snode::semantic::appLog().warn() << "Error: " << errorString << " : " << errorNumber;
                        res->status(404).send(userId + ": " + errorString + " - " + std::to_string(errorNumber));
                    });
            },
            [] MIDDLEWARE(req, res, next) {
                snode::semantic::appLog().debug() << "And again 1: Move on to the next route to send result";
                next();
            },
            [] MIDDLEWARE(req, res, next) {
                snode::semantic::appLog().debug() << "And again 2: Move on to the next route to send result";
                next();
            })
        .get([] MIDDLEWARE(req, res, next) {
            snode::semantic::appLog().debug() << "And again 3: Move on to the next route to send result";
            next();
        })
        .get([] APPLICATION(req, res) {
            snode::semantic::appLog().debug() << "SendResult";

            req->getAttribute<std::string, "html-table">(
                [res](std::string& table) {
                    res->send(table);
                },
                [res](const std::string&) {
                    res->end();
                });
        });
    router.get("/account/:userId(\\d*)/:userName", [&db] APPLICATION(req, res) { // http://localhost:8080/account/123/perfectNDSgroup
        snode::semantic::appLog().debug() << "Show account of";
        snode::semantic::appLog().debug() << "UserId: " << req->params["userId"];
        snode::semantic::appLog().debug() << "UserName: " << req->params["userName"];

        const std::string response = "<html>"
                                     "  <head>"
                                     "    <title>Response from snode.c</title>"
                                     "  </head>"
                                     "  <body>"
                                     "    <h1>Regex return</h1>"
                                     "    <ul>"
                                     "      <li>UserId: " +
                                     req->params["userId"] +
                                     "      </li>"
                                     "      <li>UserName: " +
                                     req->params["userName"] +
                                     "      </li>"
                                     "    </ul>"
                                     "  </body>"
                                     "</html>";

        const std::string userId = req->params["userId"];
        const std::string userName = req->params["userName"];

        db.exec(
            "INSERT INTO `snodec`(`username`, `password`) VALUES ('" + userId + "','" + userName + "')",
            [userId, userName]() {
                snode::semantic::appLog().debug() << "Inserted: -> " << userId << " - " << userName;
            },
            [](const std::string& errorString, unsigned int errorNumber) {
                snode::semantic::appLog().warn() << "Error: " << errorString << " : " << errorNumber;
            });

        res->send(response);
    });
    router.get("/asdf/:testRegex1(d\\d{3}e)/jklö/:testRegex2", [] APPLICATION(req, res) { // http://localhost:8080/asdf/d123e/jklö/hallo
        snode::semantic::appLog().debug() << "Testing Regex";
        snode::semantic::appLog().debug() << "Regex1: " << req->params["testRegex1"];
        snode::semantic::appLog().debug() << "Regex2: " << req->params["testRegex2"];

        const std::string response = "<html>"
                                     "  <head>"
                                     "    <title>Response from snode.c</title>"
                                     "  </head>"
                                     "  <body>"
                                     "    <h1>Regex return</h1>"
                                     "    <ul>"
                                     "      <li>Regex 1: " +
                                     req->params["testRegex1"] +
                                     "      </li>"
                                     "      <li>Regex 2: " +
                                     req->params["testRegex2"] +
                                     "      </li>"
                                     "    </ul>"
                                     "  </body>"
                                     "</html>";

        res->send(response);
    });
    router.get("/search/:search", [] APPLICATION(req, res) { // http://localhost:8080/search/buxtehude123
        snode::semantic::appLog().debug() << "Show Search of";
        snode::semantic::appLog().debug() << "Search: " << req->params["search"];
        snode::semantic::appLog().debug() << "Queries: " << req->query("test");

        res->send(req->params["search"]);
    });
    router.use([] APPLICATION(req, res) {
        res->status(404).send("Not found: " + req->url);
    });

    return router;
}

int main(int argc, char* argv[]) {
    WebApp::init(argc, argv);

    const database::mariadb::MariaDBConnectionDetails details = {
        .connectionName = "regex",
        .hostname = "localhost",
        .username = "snodec",
        .password = "pentium5",
        .database = "snodec",
        .port = 3306,
        .socket = "/run/mysqld/mysqld.sock",
        .flags = 0,
    };

    // CREATE USER 'snodec'@localhost IDENTIFIED BY 'pentium5'
    // GRANT ALL PRIVILEGES ON *.* TO 'snodec'@localhost
    // GRANT ALL PRIVILEGES ON 'snodec'.'snodec' TO 'snodec'@localhost
    // CREATE DATABASE 'snodec';
    // CREATE TABLE 'snodec' ('username' text NOT NULL, 'password' text NOT NULL ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4

    database::mariadb::MariaDBClient db(details, [](const database::mariadb::MariaDBState& state) {
        if (state.error != 0) {
            snode::semantic::appLog().error() << "MySQL error: " << state.errorMessage << " [" << state.error << "]";
        } else if (state.connected) {
            snode::semantic::mariaDbLog().info() << "MySQL connected";
        } else {
            snode::semantic::mariaDbLog().info() << "MySQL disconnected";
        }
    });

    {
        legacy::in::WebApp legacyApp("legacy-testregex");

        legacyApp.use(router(db));

        legacyApp.listen(8080, [](const legacy::in::WebApp::SocketAddress& socketAddress, const core::socket::State& state) {
            switch (state) {
                case core::socket::State::OK:
                    snode::semantic::appLog().info() << "legacy-testregex: listening on '" << socketAddress.toString() << "'";
                    break;
                case core::socket::State::DISABLED:
                    snode::semantic::appLog().info() << "legacy-testregex: disabled";
                    break;
                case core::socket::State::ERROR:
                    snode::semantic::appLog().warn() << "legacy-testregex: error occurred";
                    break;
                case core::socket::State::FATAL:
                    snode::semantic::appLog().error() << "legacy-testregex: fatal error occurred";
                    break;
            }
        });

        legacyApp.setOnConnect([](legacy::in::WebApp::SocketConnection* socketConnection) {
            snode::semantic::appLog().debug() << "OnConnect:";

            snode::semantic::appLog().debug() << "\tServer: " + socketConnection->getRemoteAddress().toString();
            snode::semantic::appLog().debug() << "\tClient: " + socketConnection->getLocalAddress().toString();
        });

        legacyApp.setOnDisconnect([](legacy::in::WebApp::SocketConnection* socketConnection) {
            snode::semantic::appLog().debug() << "OnDisconnect:";

            snode::semantic::appLog().debug() << "\tServer: " + socketConnection->getRemoteAddress().toString();
            snode::semantic::appLog().debug() << "\tClient: " + socketConnection->getLocalAddress().toString();
        });

        tls::in::WebApp tlsApp("tls-testregex");

        tlsApp.use(legacyApp);

        tlsApp.listen(8088, [](const tls::in::WebApp::SocketAddress& socketAddress, const core::socket::State& state) {
            switch (state) {
                case core::socket::State::OK:
                    snode::semantic::appLog().info() << "tls-testregex: listening on '" << socketAddress.toString() << "'";
                    break;
                case core::socket::State::DISABLED:
                    snode::semantic::appLog().info() << "tls-testregex: disabled";
                    break;
                case core::socket::State::ERROR:
                    snode::semantic::appLog().warn() << "tls-testregex: error occurred";
                    break;
                case core::socket::State::FATAL:
                    snode::semantic::appLog().error() << "tls-testregex: fatal error occurred";
                    break;
            }
        });

        tlsApp.setOnConnect([](tls::in::WebApp::SocketConnection* socketConnection) {
            snode::semantic::appLog().debug() << "OnConnect:";

            snode::semantic::appLog().debug() << "\tServer: " + socketConnection->getRemoteAddress().toString();
            snode::semantic::appLog().debug() << "\tClient: " + socketConnection->getLocalAddress().toString();
        });

        tlsApp.setOnConnected([](tls::in::WebApp::SocketConnection* socketConnection) {
            snode::semantic::appLog().debug() << "OnConnected:";

            auto log = snode::semantic::appLog();
            if (log.enabled(logger::LogLevel::Debug)) {
                X509* client_cert = SSL_get_peer_certificate(socketConnection->getSSL());

                if (client_cert != nullptr) {
                    const long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                    snode::semantic::appLog().debug() << "\tClient certificate: " + std::string(X509_verify_cert_error_string(verifyErr));

                    char* str = X509_NAME_oneline(X509_get_subject_name(client_cert), nullptr, 0);
                    if (str != nullptr) {
                        snode::semantic::appLog().debug() << "\t   Subject: " << str;
                        OPENSSL_free(str);
                    }

                    str = X509_NAME_oneline(X509_get_issuer_name(client_cert), nullptr, 0);
                    if (str != nullptr) {
                        snode::semantic::appLog().debug() << "\t   Issuer: " << str;
                        OPENSSL_free(str);
                    }

                    // We could do all sorts of certificate verification stuff here before deallocating the certificate.

                    GENERAL_NAMES* subjectAltNames =
                        static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(client_cert, NID_subject_alt_name, nullptr, nullptr));

                    const int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);

                    snode::semantic::appLog().debug() << "\t   Subject alternative name count: " << altNameCount;
                    for (int32_t i = 0; i < altNameCount; ++i) {
                        GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
                        if (generalName->type == GEN_URI) {
                            const std::string subjectAltName =
                                std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),
                                            static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));
                            snode::semantic::appLog().debug() << "\t      SAN (URI): '" + subjectAltName;
                        } else if (generalName->type == GEN_DNS) {
                            const std::string subjectAltName =
                                std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                            static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));
                            snode::semantic::appLog().debug() << "\t      SAN (DNS): '" + subjectAltName;
                        } else {
                            snode::semantic::appLog().debug() << "\t      SAN (Type): '" + std::to_string(generalName->type);
                        }
                    }

                    sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);

                    X509_free(client_cert);
                } else {
                    snode::semantic::appLog().debug() << "\tClient certificate: no certificate";
                }
            }
        });

        tlsApp.setOnDisconnect([](tls::in::WebApp::SocketConnection* socketConnection) {
            snode::semantic::appLog().debug() << "OnDisconnect:";

            snode::semantic::appLog().debug() << "\tServer: " + socketConnection->getRemoteAddress().toString();
            snode::semantic::appLog().debug() << "\tClient: " + socketConnection->getLocalAddress().toString();
        });
    }

    return WebApp::start();
}
