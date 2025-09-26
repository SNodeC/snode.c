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
    VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName() << " HTTP response: " << req->method << " " << req->url
            << " HTTP/" << req->httpMajor << "." << req->httpMinor << "\n"
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
                VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName() << ": OnRequestStart";

                req->httpMinor = 0;
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->setTrailer("MyTrailer",
                                "MyTrailerValue"); // The "Trailer" header field should be populated but the Trailer itself should not be
                                                   // send here because there is no content which is send using "Transfer-Encoding:chunked"
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });

                req->httpMinor = 1;
                req->method = "GET";
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->sendFile(
                    "/home/voc/projects/snodec/snode.c/CMakeLists.txt",
                    [req](int ret) {
                        if (ret == 0) {
                            VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName()
                                    << " HTTP: Request accepted: POST / HTTP/" << req->httpMajor << "." << req->httpMinor;
                            VLOG(1) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
                        } else {
                            LOG(ERROR) << req->getSocketContext()->getSocketConnection()->getConnectionName()
                                       << " HTTP: Request failed: POST / HTTP/" << req->httpMajor << "." << req->httpMinor;
                            PLOG(ERROR) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";

                            req->set("Connection", "close");
                            req->end(
                                []([[maybe_unused]] const std::shared_ptr<Response>& res) {
                                },
                                [req](const std::string&) {
                                });
                        }
                    },
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });

                req->init();
                req->httpMinor = 1;
                req->url = "/index.hhh";
                req->set("Connection", "close");
                req->setTrailer("MyTrailer",
                                "MyTrailerValue"); // The "Trailer" header field should be populated but the Trailer itself should not be
                                                   // send here because there is no content which is send using "Transfer-Encoding:chunked"
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });

// #define LONG
#ifdef LONG
                req->url = "/hihihih";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });

                req->httpMinor = 1;
                req->url = "/index.html";
                //                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });

                req->httpMinor = 1;
                req->method = "POST";
                req->url = "/";
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
                        if (ret == 0) {
                            VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName()
                                    << " HTTP: Request accepted: POST / HTTP/" << req->httpMajor << "." << req->httpMinor;
                            VLOG(1) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
                        } else {
                            LOG(ERROR) << req->getSocketContext()->getSocketConnection()->getConnectionName()
                                       << " HTTP: Request failed: POST / HTTP/" << req->httpMajor << "." << req->httpMinor;
                            PLOG(ERROR) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";

                            req->set("Connection", "close");
                            req->end(
                                [req](const std::shared_ptr<Response>& res) {
                                    logResponse(req, res);
                                },
                                [req](const std::string&) {
                                });
                        }
                    },
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);

                        req->method = "POST";
                        req->url = "/";
                        req->set("Connection", "keep-alive");
                        req->set("Test", "bbb");
                        req->sendFile(
                            "/home/voc/projects/snodec/snode.c/CMakeLists.tt",
                            [req](int ret) {
                                if (ret == 0) {
                                    VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName()
                                            << " HTTP: Request accepted: POST / HTTP/" << req->httpMajor << "." << req->httpMinor;
                                    VLOG(1) << "  /home/voc/projects/snodec/snode.c/CMakeLists.tt";
                                } else {
                                    LOG(ERROR) << req->getSocketContext()->getSocketConnection()->getConnectionName()
                                               << " HTTP: Request failed: POST / HTTP/" << req->httpMajor << "." << req->httpMinor;
                                    PLOG(ERROR) << "  /home/voc/projects/snodec/snode.c/CMakeLists.tt";

                                    //                                    req->init();
                                    req->method = "GET";
                                    req->url = "/";
                                    req->set("Connection", "close");
                                    req->set("Test", "ccc");
                                    req->end(
                                        [req](const std::shared_ptr<Response>& res) {
                                            logResponse(req, res);
                                        },
                                        [](const std::string&) {
                                        });
                                }
                            },
                            [req](const std::shared_ptr<Response>& res) {
                                logResponse(req, res);
                            },
                            [req](const std::string&) {
                            });
                    },
                    [req](const std::string&) {
                    });
                req->method = "GET";
                req->url = "/";
                req->set("Connection", "close");
                req->set("Test", "xxx");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                core::EventReceiver::atNextTick([req]() {
                    req->method = "POST";
                    req->url = "/";
                    req->set("Connection", "keep-alive");
                    req->set("Test", "ddd");
                    req->sendFile(
                        "/home/voc/projects/snodec/snode.c/CMakeLists.txt",
                        [req](int ret) {
                            if (ret == 0) {
                                VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName()
                                        << " HTTP: Request accepted: POST / HTTP/" << req->httpMajor << "." << req->httpMinor;
                                VLOG(1) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
                            } else {
                                LOG(ERROR) << req->getSocketContext()->getSocketConnection()->getConnectionName()
                                           << " HTTP: Request failed: POST / HTTP/" << req->httpMajor << "." << req->httpMinor;
                                PLOG(ERROR) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";

                                req->set("Connection", "close");
                                req->end(
                                    []([[maybe_unused]] const std::shared_ptr<Response>& res) {
                                    },
                                    [req](const std::string&) {
                                    });
                            }
                        },
                        [req](const std::shared_ptr<Response>& res) {
                            logResponse(req, res);
                        },
                        [req](const std::string&) {
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
                            if (ret == 0) {
                                VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName()
                                        << " HTTP: Request accepted: POST / HTTP/" << req->httpMajor << "." << req->httpMinor;
                                VLOG(1) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";
                            } else {
                                LOG(ERROR) << req->getSocketContext()->getSocketConnection()->getConnectionName()
                                           << " HTTP: Request failed: POST / HTTP/" << req->httpMajor << "." << req->httpMinor;
                                PLOG(ERROR) << "  /home/voc/projects/snodec/snode.c/CMakeLists.txt";

                                req->set("Connection", "close");
                                req->end(
                                    []([[maybe_unused]] const std::shared_ptr<Response>& res) {
                                    },
                                    [req](const std::string&) {
                                    });
                            }
                        },
                        [req](const std::shared_ptr<Response>& res) {
                            logResponse(req, res);
                        },
                        [req](const std::string&) {
                        });
                });
#endif
            },
            []([[maybe_unused]] const std::shared_ptr<Request>& req) {
                VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName() << ": OnRequestEnd";
            });

        client.setOnConnect([](SocketConnection* socketConnection) { // onConnect
            VLOG(1) << socketConnection->getConnectionName() << ": OnConnect";

            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
        });

        client.setOnDisconnect([](SocketConnection* socketConnection) { // onDisconnect
            VLOG(1) << socketConnection->getConnectionName() << ": OnDisconnect";

            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
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
                VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName() << ": OnRequestStart";

                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/index.html";
                req->set("Connection", "keep-alive");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
                req->url = "/";
                req->set("Connection", "close");
                req->end(
                    [req](const std::shared_ptr<Response>& res) {
                        logResponse(req, res);
                    },
                    [req](const std::string&) {
                    });
            },
            []([[maybe_unused]] const std::shared_ptr<Request>& req) {
                VLOG(1) << req->getSocketContext()->getSocketConnection()->getConnectionName() << ": OnRequestEnd";
            });

        client.setOnConnect([](SocketConnection* socketConnection) { // onConnect
            VLOG(1) << "OnConnect " << socketConnection->getConnectionName();

            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();

            /* Enable automatic hostname checks */
            // X509_VERIFY_PARAM* param = SSL_get0_param(socketConnection->getSSL());

            // X509_VERIFY_PARAM_set_hostflags(param, X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS);
            // if (!X509_VERIFY_PARAM_set1_host(param, "localhost", sizeof("localhost") - 1)) {
            //   // handle error
            //   socketConnection->close();
            // }
        });

        client.setOnConnected([](SocketConnection* socketConnection) { // onConnected
            VLOG(1) << socketConnection->getConnectionName() << ": OnConnected";
            X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
            if (server_cert != nullptr) {
                long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                VLOG(1) << "\tPeer certificate verifyErr = " + std::to_string(verifyErr) + ": " +
                               std::string(X509_verify_cert_error_string(verifyErr));

                char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), nullptr, 0);
                VLOG(1) << "\t   Subject: " + std::string(str);
                OPENSSL_free(str);

                str = X509_NAME_oneline(X509_get_issuer_name(server_cert), nullptr, 0);
                VLOG(1) << "\t   Issuer: " + std::string(str);
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
                VLOG(1) << "\t   Subject alternative name count: " << altNameCount;
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
                        VLOG(1) << "\t      SAN (URI): '" + subjectAltName;
                    } else if (generalName->type == GEN_DNS) {
                        std::string subjectAltName =
                            std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                        static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));
                        VLOG(1) << "\t      SAN (DNS): '" + subjectAltName;
                    } else {
                        VLOG(1) << "\t      SAN (Type): '" + std::to_string(generalName->type);
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
                VLOG(1) << "\tPeer certificate: no certificate";
            }
        });

        client.setOnDisconnect([](SocketConnection* socketConnection) { // onDisconnect
            VLOG(1) << socketConnection->getConnectionName() << ": OnDisconnect";

            VLOG(1) << "\tLocal: " << socketConnection->getLocalAddress().toString();
            VLOG(1) << "\tPeer:  " << socketConnection->getRemoteAddress().toString();
        });

        return client;
    }

} // namespace apps::http::tls

#endif // (STREAM_TYPE == TLS) // tls

#endif // APPS_HTTP_MODEL_CLIENTS_H
