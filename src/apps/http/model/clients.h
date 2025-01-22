/*
 * SNode.C - a slim toolkit for network communication
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

#ifndef APPS_HTTP_MODEL_CLIENTS_H
#define APPS_HTTP_MODEL_CLIENTS_H

#define QUOTE_INCLUDE(a) STR_INCLUDE(a)
#define STR_INCLUDE(a) #a

// clang-format off
#define CLIENT_INCLUDE QUOTE_INCLUDE(web/http/STREAM/NET/Client.h)
// clang-format on

#include CLIENT_INCLUDE // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "web/http/http_utils.h"

#if (STREAM_TYPE == TLS) // tls
#include <cstddef>
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

static void logResponse(const std::shared_ptr<web::http::client::Request>& req, const std::shared_ptr<web::http::client::Response>& res) {
    LOG(INFO) << req->getSocketContext()->getSocketConnection()->getInstanceName() << ": OnResponse";
    LOG(INFO) << "   Request: " << req->method << " " << req->url << " HTTP/" << req->httpMajor << "." << req->httpMinor << "\n"
              << httputils::toString(req->method,
                                     req->url,
                                     "HTTP/" + std::to_string(req->httpMajor) + "." + std::to_string(req->httpMinor),
                                     req->getQueries(),
                                     req->getHeaders(),
                                     req->getCookies(),
                                     {})
              << "\n"
              << httputils::toString(res->httpVersion, res->statusCode, res->reason, res->headers, res->cookies, res->body);
}

#if (STREAM_TYPE == LEGACY) // legacy

namespace apps::http::legacy {

    using Client = web::http::legacy::NET::Client;
    using Request = Client::Request;
    using Response = Client::Response;
    using SocketConnection = Client::SocketConnection;

    static Client getClient() {
        Client client(
            "httpclient",
            [](const std::shared_ptr<Request>& req) {
                LOG(INFO) << req->getSocketContext()->getSocketConnection()->getInstanceName() << ": OnRequestStart";

                req->httpMinor = 0;
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->setTrailer("MyTrailer",
                                "MyTrailerValue"); // The "Trailer" header field should be populated but the Trailer itself should not be
                                                   // send here because there is no content which is send using "Transfer-Encoding:chunked"
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
#define LONG
#ifdef LONG
                req->url = "/hihihih";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });

                req->httpMinor = 1;
                req->url = "/index.html";
                //                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });

                req->httpMinor = 1;
                req->method = "POST";
                req->url = "/";
                //                req->set("Connection", "keep-alive");
                req->set("Test", "aaa");
                req->setTrailer("MyTrailer1",
                                "MyTrailerValue1"); // Full trailer processing. Header field "Trailer" set and the Trailer itself is also
                                                    // sent because here content will be sent with "Transfer-Encoding:chunked"
                req->setTrailer("MyTrailer2", "MyTrailerValue2");
                req->setTrailer("MyTrailer3", "MyTrailerValue3");
                req->setTrailer("MyTrailer4", "MyTrailerValue4");
                req->setTrailer("MyTrailer5", "MyTrailerValue5");
                req->setTrailer("MyTrailer6", "MyTrailerValue6");
                req->query("Query1", "QueryValue1");
                req->query("Query2", "QueryValue2");
                req->sendFile(
                    "/home/voc/projects/snodec/snode.c/CMakeLists.txt",
                    [req](int ret) {
                        PLOG(INFO) << "HTTP-Client: POST /home/voc/projects/snodec/snode.c/CMakeLists.txt";

                        if (ret == 0) {
                            LOG(INFO) << "Send /home/voc/projects/snodec/snode.c/CMakeLists.txt started";
                        } else {
                            LOG(INFO) << "Send /home/voc/projects/snodec/snode.c/CMakeLists.txt failed";

                            req->set("Connection", "close");
                            req->end([]([[maybe_unused]] const std::shared_ptr<Request>& req,
                                        [[maybe_unused]] const std::shared_ptr<Response>& res) {
                            });
                        }
                    },
                    [](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                        logResponse(req, res);

                        req->init();
                        req->method = "POST";
                        req->url = "/";
                        req->set("Connection", "keep-alive");
                        req->set("Test", "bbb");
                        req->sendFile(
                            "/home/voc/projects/snodec/snode.c/CMakeLists.tt",
                            [req](int ret) {
                                if (ret == 0) {
                                    LOG(INFO) << "HTTP-Client: POST  /home/voc/projects/snodec/snode.c/CMakeLists.tt started";
                                } else {
                                    PLOG(INFO) << "HTTP-Client: POST  /home/voc/projects/snodec/snode.c/CMakeLists.tt failed";

                                    req->init();
                                    req->method = "GET";
                                    req->url = "/";
                                    req->set("Connection", "close");
                                    req->set("Test", "ccc");
                                    req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                                        logResponse(req, res);
                                    });
                                }
                            },
                            [](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                                logResponse(req, res);
                            });
                    });
                core::EventReceiver::atNextTick([req]() {
                    req->method = "POST";
                    req->url = "/";
                    req->set("Connection", "keep-alive");
                    req->set("Test", "ddd");
                    req->sendFile(
                        "/home/voc/projects/snodec/snode.c/CMakeLists.txt",
                        [req](int ret) {
                            PLOG(INFO) << "HTTP-Client: POST /home/voc/projects/snodec/snode.c/CMakeLists.txt";

                            if (ret == 0) {
                                LOG(INFO) << "Send /home/voc/projects/snodec/snode.c/CMakeLists.txt started";
                            } else {
                                LOG(INFO) << "Send /home/voc/projects/snodec/snode.c/CMakeLists.txt failed";
                            }
                        },
                        [](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                            logResponse(req, res);
                        });

                    req->method = "POST";
                    req->url = "/";
                    req->set("Connection", "keep-alive");
                    req->set("Test", "eee");
                    req->setTrailer("MyTrailer1", "MyTrailerValue1");
                    req->setTrailer("MyTrailer2", "MyTrailerValue2");
                    req->sendFile(
                        "/home/voc/projects/snodec/snode.c/CMakeLists.txt",
                        [req](int ret) {
                            PLOG(INFO) << "HTTP-Client: POST /home/voc/projects/snodec/snode.c/CMakeLists.txt ";

                            if (ret == 0) {
                                LOG(INFO) << "Send /home/voc/projects/snodec/snode.c/CMakeLists.txt started ";
                            } else {
                                LOG(INFO) << " Send /home/voc/projects/snodec/snode.c/CMakeLists.txt failed";
                            }
                        },
                        [](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                            logResponse(req, res);
                        });
                });
#endif
            },
            []([[maybe_unused]] const std::shared_ptr<Request>& req) {
                LOG(INFO) << req->getSocketContext()->getSocketConnection()->getInstanceName() << ": OnRequestEnd";
            });

        client.setOnConnect([](SocketConnection* socketConnection) { // onConnect
            LOG(INFO) << socketConnection->getInstanceName() << ": OnConnect";

            LOG(INFO) << "\tLocal: " << socketConnection->getLocalAddress().toString();
            LOG(INFO) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
        });

        client.setOnDisconnect([](SocketConnection* socketConnection) { // onDisconnect
            LOG(INFO) << socketConnection->getInstanceName() << ": OnDisconnect";

            LOG(INFO) << "\tLocal: " << socketConnection->getLocalAddress().toString();
            LOG(INFO) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
        });

        return client;
    }

} // namespace apps::http::legacy

#endif // (STREAM_TYPE == LEGACY) // legacy

#if (STREAM_TYPE == TLS) // tls

namespace apps::http::tls {

    using Client = web::http::tls::NET::Client;
    using Request = Client::Request;
    using Response = Client::Response;
    using SocketConnection = Client::SocketConnection;

    static Client getClient() {
        Client client(
            "httpclient",
            [](const std::shared_ptr<Request>& req) {
                LOG(INFO) << req->getSocketContext()->getSocketConnection()->getInstanceName() << ": OnRequestStart";

                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
                req->url = "/";
                req->set("Connection", "close");
                req->end([](const std::shared_ptr<Request>& req, const std::shared_ptr<Response>& res) {
                    logResponse(req, res);
                });
            },
            []([[maybe_unused]] const std::shared_ptr<Request>& req) {
                LOG(INFO) << req->getSocketContext()->getSocketConnection()->getInstanceName() << ": OnRequestEnd";
            });

        client.setOnConnect([](SocketConnection* socketConnection) { // onConnect
            LOG(INFO) << "OnConnect " << socketConnection->getInstanceName();

            LOG(INFO) << "\tLocal: " << socketConnection->getLocalAddress().toString();
            LOG(INFO) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();

            /* Enable automatic hostname checks */
            // X509_VERIFY_PARAM* param = SSL_get0_param(socketConnection->getSSL());

            // X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
            // if (!X509_VERIFY_PARAM_set1_host(param, "localhost", sizeof("localhost") - 1)) {
            //   // handle error
            //   socketConnection->close();
            // }
        });

        client.setOnConnected([](SocketConnection* socketConnection) { // onConnected
            LOG(INFO) << socketConnection->getInstanceName() << ": OnConnected";
            X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
            if (server_cert != nullptr) {
                long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                LOG(INFO) << "\tPeer certificate verifyErr = " + std::to_string(verifyErr) + ": " +
                                 std::string(X509_verify_cert_error_string(verifyErr));

                char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);
                LOG(INFO) << "\t   Subject: " + std::string(str);
                OPENSSL_free(str);

                str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);
                LOG(INFO) << "\t   Issuer: " + std::string(str);
                OPENSSL_free(str);

                // We could do all sorts of certificate verification stuff here before deallocating the certificate.

                GENERAL_NAMES* subjectAltNames =
                    static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(server_cert, NID_subject_alt_name, nullptr, nullptr));
#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wused-but-marked-unused")
#pragma GCC diagnostic ignored "-Wused-but-marked-unused"
#endif
#endif
#endif
                int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);
#ifdef __GNUC_
#pragma GCC diagnostic pop
#endif
                VLOG(0) << "\t   Subject alternative name count: " << altNameCount;
                for (int32_t i = 0; i < altNameCount; ++i) {
#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wused-but-marked-unused")
#pragma GCC diagnostic ignored "-Wused-but-marked-unused"
#endif
#endif
#endif
                    GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
#ifdef __GNUC_
#pragma GCC diagnostic pop
#endif
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
#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wused-but-marked-unused")
#pragma GCC diagnostic ignored "-Wused-but-marked-unused"
#endif
#endif
#endif
                sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);
#ifdef __GNUC_
#pragma GCC diagnostic pop
#endif
                X509_free(server_cert);
            } else {
                LOG(INFO) << "\tPeer certificate: no certificate";
            }
        });

        client.setOnDisconnect([](SocketConnection* socketConnection) { // onDisconnect
            LOG(INFO) << socketConnection->getInstanceName() << ": OnDisconnect";

            LOG(INFO) << "\tLocal: " << socketConnection->getLocalAddress().toString();
            LOG(INFO) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
        });

        return client;
    }

} // namespace apps::http::tls

#endif // (STREAM_TYPE == TLS) // tls

#endif // APPS_HTTP_MODEL_CLIENTS_H
