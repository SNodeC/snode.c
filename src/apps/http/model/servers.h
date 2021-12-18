#ifndef APPS_HTTP_MODEL_SERVERS_H
#define APPS_HTTP_MODEL_SERVERS_H

#define QUOTE_INCLUDE(a) STR_INCLUDE(a)
#define STR_INCLUDE(a) #a

// clang-format off
#define WEBAPP_INCLUDE QUOTE_INCLUDE(express/STREAM/NET/WebApp.h)
// clang-format on

#include WEBAPP_INCLUDE // IWYU pragma: export

#include "express/Router.h"
#include "express/middleware/StaticMiddleware.h"

#if (STREAM_TYPE == TLS) // tls
#include <cstddef>       // for size_t
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#endif

#include "log/Logger.h"

express::Router getRouter(const std::string& rootPath) {
    express::Router router;

    router.use(express::middleware::StaticMiddleware(rootPath));

    return router;
}

#if (STREAM_TYPE == LEGACY) // legacy

namespace apps::http::legacy {

    using WebApp = express::legacy::NET::WebApp;

    WebApp getWebApp(const std::string& rootPath, const std::map<std::string, std::any>& options) {
        WebApp webApp(getRouter(rootPath), options);

        webApp.onConnect([](WebApp::SocketConnection* socketConnection) -> void {
            VLOG(0) << "OnConnect:";

            VLOG(0) << "\tServer: (" + socketConnection->getLocalAddress().address() + ") " +
                           socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tClient: (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();
        });

        webApp.onDisconnect([](WebApp::SocketConnection* socketConnection) -> void {
            VLOG(0) << "OnDisconnect:";

            VLOG(0) << "\tServer: (" + socketConnection->getLocalAddress().address() + ") " +
                           socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tClient: (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();
        });

        return webApp;
    }

} // namespace apps::http::legacy

#endif // (STREAM_TYPE == LEGACY) // legacy

#if (STREAM_TYPE == TLS) // tls

namespace apps::http::tls {

    using WebApp = express::tls::NET::WebApp;

    WebApp getWebApp(const std::string& rootPath, const std::map<std::string, std::any>& options) {
        WebApp webApp(getRouter(rootPath), options);

        webApp.onConnect([](WebApp::SocketConnection* socketConnection) -> void {
            VLOG(0) << "OnConnect:";

            VLOG(0) << "\tServer: (" + socketConnection->getLocalAddress().address() + ") " +
                           socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tClient: (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();
        });

        webApp.onConnected([](WebApp::SocketConnection* socketConnection) -> void {
            VLOG(0) << "OnConnected:";

            X509* client_cert = SSL_get_peer_certificate(socketConnection->getSSL());

            if (client_cert != nullptr) {
                long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                VLOG(2) << "\tClient certificate: " + std::string(X509_verify_cert_error_string(verifyErr));

                char* str = X509_NAME_oneline(X509_get_subject_name(client_cert), 0, 0);
                VLOG(2) << "\t   Subject: " + std::string(str);
                OPENSSL_free(str);

                str = X509_NAME_oneline(X509_get_issuer_name(client_cert), 0, 0);
                VLOG(2) << "\t   Issuer: " + std::string(str);
                OPENSSL_free(str);

                // We could do all sorts of certificate verification stuff here before deallocating the certificate.

                GENERAL_NAMES* subjectAltNames =
                    static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(client_cert, NID_subject_alt_name, nullptr, nullptr));

                int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);
                VLOG(2) << "\t   Subject alternative name count: " << altNameCount;
                for (int32_t i = 0; i < altNameCount; ++i) {
                    GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
                    if (generalName->type == GEN_URI) {
                        std::string subjectAltName =
                            std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),
                                        static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));
                        VLOG(2) << "\t      SAN (URI): '" + subjectAltName;
                    } else if (generalName->type == GEN_DNS) {
                        std::string subjectAltName =
                            std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                        static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));
                        VLOG(2) << "\t      SAN (DNS): '" + subjectAltName;
                    } else {
                        VLOG(2) << "\t      SAN (Type): '" + std::to_string(generalName->type);
                    }
                }
                sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);

                X509_free(client_cert);
            } else {
                VLOG(2) << "\tClient certificate: no certificate";
                // Here we can close the connection in case client didn't send a certificate
                // socketConnection->close();
            }
        });

        webApp.onDisconnect([](WebApp::SocketConnection* socketConnection) -> void {
            VLOG(0) << "OnDisconnect:";

            VLOG(0) << "\tServer: (" + socketConnection->getLocalAddress().address() + ") " +
                           socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tClient: (" + socketConnection->getRemoteAddress().address() + ") " +
                           socketConnection->getRemoteAddress().toString();
        });

        return webApp;
    }

} // namespace apps::http::tls

#endif // (STREAM_TYPE == TLS) // tls

#endif // APPS_HTTP_MODEL_SERVERS_H
