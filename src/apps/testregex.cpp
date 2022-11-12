/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "database/mariadb/MariaDBClient.h"
#include "express/legacy/in/WebApp.h"
#include "express/tls/in/WebApp.h"
#include "log/Logger.h"

#include <iostream>
#include <openssl/asn1.h>
#include <openssl/crypto.h>
#include <openssl/obj_mac.h>
#include <openssl/opensslv.h>
#include <openssl/ssl.h> // IWYU pragma: keep
#include <openssl/x509.h>
#include <openssl/x509v3.h>

// IWYU pragma: no_include <openssl/ssl3.h>

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/types.h>
#elif OPENSSL_VERSION_NUMBER >= 0x10100000L
#include <openssl/ossl_typ.h>
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

Router router(database::mariadb::MariaDBClient& db) {
    Router router;

    router.get("/test/:variable(\\d)/:uri", [] APPLICATION(req, res) { // http://localhost:8080/test/1/urlstring
        std::cout << "TEST" << std::endl;
    });
    router
        .get(
            "/query/:userId",
            [] MIDDLEWARE(req, res, next) {
                VLOG(0) << "Move on to the next route to query database";
                next();
            },
            [&db] MIDDLEWARE(req, res, next) { // http://localhost:8080/query/123
                VLOG(0) << "UserId: " << req.params["userId"];
                std::string userId = req.params["userId"];

                req.setAttribute<std::string, "html-table">(std::string());

                req.getAttribute<std::string, "html-table">([&userId](std::string& table) -> void {
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
                    [next, &req, i](const MYSQL_ROW row) mutable -> void {
                        if (row != nullptr) {
                            i++;
                            req.getAttribute<std::string, "html-table">([row, &i](std::string& table) -> void {
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
                            req.getAttribute<std::string, "html-table">([](std::string& table) -> void {
                                table.append(std::string("    </table>\n"
                                                         "  </body>\n"
                                                         "</html>\n"));
                            });
                            VLOG(0) << "Move on to the next route to send result";
                            next();
                        }
                    },
                    [&res, userId](const std::string& errorString, unsigned int errorNumber) -> void {
                        VLOG(0) << "Error: " << errorString << " : " << errorNumber;
                        res.status(404).send(userId + ": " + errorString + " - " + std::to_string(errorNumber));
                    });
            },
            [] MIDDLEWARE(req, res, next) {
                VLOG(0) << "And again 1: Move on to the next route to send result";
                next();
            },
            [] MIDDLEWARE(req, res, next) {
                VLOG(0) << "And again 2: Move on to the next route to send result";
                next();
            })
        .get([] MIDDLEWARE(req, res, next) {
            VLOG(0) << "And again 3: Move on to the next route to send result";
            next();
        })
        .get([] APPLICATION(req, res) {
            VLOG(0) << "SendResult";

            req.getAttribute<std::string, "html-table">(
                [&res](std::string& table) -> void {
                    res.send(table);
                },
                [&res](const std::string&) -> void {
                    res.end();
                });
        });
    router.get("/account/:userId(\\d*)/:userName", [&db] APPLICATION(req, res) { // http://localhost:8080/account/123/perfectNDSgroup
        VLOG(0) << "Show account of";
        VLOG(0) << "UserId: " << req.params["userId"];
        VLOG(0) << "UserName: " << req.params["userName"];

        std::string response = "<html>"
                               "  <head>"
                               "    <title>Response from snode.c</title>"
                               "  </head>"
                               "  <body>"
                               "    <h1>Regex return</h1>"
                               "    <ul>"
                               "      <li>UserId: " +
                               req.params["userId"] +
                               "      </li>"
                               "      <li>UserName: " +
                               req.params["userName"] +
                               "      </li>"
                               "    </ul>"
                               "  </body>"
                               "</html>";

        std::string userId = req.params["userId"];
        std::string userName = req.params["userName"];

        db.exec(
            "INSERT INTO `snodec`(`username`, `password`) VALUES ('" + userId + "','" + userName + "')",
            [userId, userName](void) -> void {
                VLOG(0) << "Inserted: -> " << userId << " - " << userName;
            },
            [](const std::string& errorString, unsigned int errorNumber) -> void {
                VLOG(0) << "Error: " << errorString << " : " << errorNumber;
            });

        res.send(response);
    });
    router.get("/asdf/:testRegex1(d\\d{3}e)/jklö/:testRegex2", [] APPLICATION(req, res) { // http://localhost:8080/asdf/d123e/jklö/hallo
        VLOG(0) << "Testing Regex";
        VLOG(0) << "Regex1: " << req.params["testRegex1"];
        VLOG(0) << "Regex2: " << req.params["testRegex2"];

        std::string response = "<html>"
                               "  <head>"
                               "    <title>Response from snode.c</title>"
                               "  </head>"
                               "  <body>"
                               "    <h1>Regex return</h1>"
                               "    <ul>"
                               "      <li>Regex 1: " +
                               req.params["testRegex1"] +
                               "      </li>"
                               "      <li>Regex 2: " +
                               req.params["testRegex2"] +
                               "      </li>"
                               "    </ul>"
                               "  </body>"
                               "</html>";

        res.send(response);
    });
    router.get("/search/:search", [] APPLICATION(req, res) { // http://localhost:8080/search/buxtehude123
        VLOG(0) << "Show Search of";
        VLOG(0) << "Search: " << req.params["search"];
        VLOG(0) << "Queries: " << req.query("test");

        res.send(req.params["search"]);
    });
    router.use([] APPLICATION(req, res) {
        res.status(404).send("Not found: " + req.url);
    });

    return router;
}

int main(int argc, char* argv[]) {
    WebApp::init(argc, argv);

    database::mariadb::MariaDBConnectionDetails details = {
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

    database::mariadb::MariaDBClient db(details);

    {
        legacy::in::WebApp legacyApp("legacy-testregex");

        legacyApp.use(router(db));

        legacyApp.listen(8080, [](const legacy::in::WebApp::SocketAddress& socketAddress, int errnum) -> void {
            if (errnum < 0) {
                PLOG(ERROR) << "OnError";
            } else if (errnum > 0) {
                PLOG(ERROR) << "OnError: " << socketAddress.toString();
            } else {
                VLOG(0) << "snode.c listening on " << socketAddress.toString();
            }
        });

        legacyApp.onConnect([](legacy::in::WebApp::SocketConnection* socketConnection) -> void {
            VLOG(0) << "OnConnect:";

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
        });

        legacyApp.onDisconnect([](legacy::in::WebApp::SocketConnection* socketConnection) -> void {
            VLOG(0) << "OnDisconnect:";

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
        });

        tls::in::WebApp tlsApp("tls-testregex");

        tlsApp.use(legacyApp);

        tlsApp.listen(8088, [](const tls::in::WebApp::SocketAddress& socketAddress, int errnum) -> void {
            if (errnum < 0) {
                PLOG(ERROR) << "OnError";
            } else if (errnum > 0) {
                PLOG(ERROR) << "OnError: " << socketAddress.toString();
            } else {
                VLOG(0) << "snode.c listening on " << socketAddress.toString();
            }
        });

        tlsApp.onConnect([](tls::in::WebApp::SocketConnection* socketConnection) -> void {
            VLOG(0) << "OnConnect:";

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
        });

        tlsApp.onConnected([](tls::in::WebApp::SocketConnection* socketConnection) {
            VLOG(0) << "OnConnected:";

            X509* client_cert = SSL_get_peer_certificate(socketConnection->getSSL());

            if (client_cert != nullptr) {
                long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                VLOG(0) << "\tClient certificate: " + std::string(X509_verify_cert_error_string(verifyErr));

                char* str = X509_NAME_oneline(X509_get_subject_name(client_cert), nullptr, 0);
                VLOG(0) << "\t   Subject: " + std::string(str);
                OPENSSL_free(str);

                str = X509_NAME_oneline(X509_get_issuer_name(client_cert), nullptr, 0);
                VLOG(0) << "\t   Issuer: " + std::string(str);
                OPENSSL_free(str);

                // We could do all sorts of certificate verification stuff here before deallocating the certificate.

                GENERAL_NAMES* subjectAltNames =
                    static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(client_cert, NID_subject_alt_name, nullptr, nullptr));

                int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);
                VLOG(0) << "\t   Subject alternative name count: " << altNameCount;
                for (int32_t i = 0; i < altNameCount; ++i) {
                    GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
                    if (generalName->type == GEN_URI) {
                        std::string subjectAltName =
                            std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),
                                        static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));
                        VLOG(0) << "\t      SAN (URI): '" + subjectAltName;
                    } else if (generalName->type == GEN_DNS) {
                        std::string subjectAltName =
                            std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                        static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));
                        VLOG(0) << "\t      SAN (DNS): '" + subjectAltName;
                    } else {
                        VLOG(0) << "\t      SAN (Type): '" + std::to_string(generalName->type);
                    }
                }
                sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);

                X509_free(client_cert);
            } else {
                VLOG(0) << "\tClient certificate: no certificate";
            }
        });

        tlsApp.onDisconnect([](tls::in::WebApp::SocketConnection* socketConnection) -> void {
            VLOG(0) << "OnDisconnect:";

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
        });
    }

    return WebApp::start();
}
