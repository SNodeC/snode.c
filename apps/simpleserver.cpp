/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "legacy/WebApp.h"
#include "middleware/StaticMiddleware.h"
#include "tls/WebApp.h"

#include <easylogging++.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CERTF "/home/voc/projects/ServerVoc/certs/calisto.home.vchrist.at_-_snode.c_-_server.pem"
#define KEYF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c_-_server.key.encrypted.pem"
#define KEYFPASS "snode.c"
#define CLIENTCAFILE "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Root_CA.crt"

#define SERVERROOT "/home/voc/projects/ServerVoc/doc/html"

using namespace express;

int main(int argc, char** argv) {
    WebApp::init(argc, argv);

    legacy::WebApp legacyApp;

    legacyApp.use(StaticMiddleware(SERVERROOT));

    legacyApp.listen(8080, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8080";
        } else {
            VLOG(0) << "snode.c listening on port 8080 for legacy connections";
        }
    });

    legacyApp.onConnect([](net::socket::legacy::SocketConnection* socketConnection) -> void {
        VLOG(0) << "Connect: " + socketConnection->getRemoteAddress().host() + ":" +
                       std::to_string(socketConnection->getRemoteAddress().port());
    });

    legacyApp.onDisconnect([](net::socket::legacy::SocketConnection* socketConnection) -> void {
        VLOG(0) << "Disconnect: " + socketConnection->getRemoteAddress().host() + ":" +
                       std::to_string(socketConnection->getRemoteAddress().port());
    });

    tls::WebApp tlsApp(CERTF, KEYF, KEYFPASS, CLIENTCAFILE);

    tlsApp.use(StaticMiddleware(SERVERROOT));

    tlsApp.listen(8088, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8088";
        } else {
            VLOG(0) << "snode.c listening on port 8088 for SSL/TLS connections";
        }
    });

    tlsApp.onConnect([](net::socket::tls::SocketConnection* socketConnection) -> void {
        VLOG(0) << "Connect: " + socketConnection->getRemoteAddress().host() + ":" +
                       std::to_string(socketConnection->getRemoteAddress().port());

        X509* client_cert = SSL_get_peer_certificate(socketConnection->getSSL());
        if (client_cert != NULL) {
            VLOG(0) << "Client certificate";

            char* str = X509_NAME_oneline(X509_get_subject_name(client_cert), 0, 0);
            VLOG(0) << "\t subject: " + std::string(str);
            OPENSSL_free(str);

            str = X509_NAME_oneline(X509_get_issuer_name(client_cert), 0, 0);
            VLOG(0) << "\t issuer: " + std::string(str);
            OPENSSL_free(str);

            // We could do all sorts of certificate verification stuff here before deallocating the certificate.

            X509_free(client_cert);

            int verifyErr = SSL_get_verify_result(socketConnection->getSSL());
            VLOG(0) << "Certificate verify result: " + std::string(X509_verify_cert_error_string(verifyErr));
        } else {
            VLOG(0) << "Client \"" + socketConnection->getRemoteAddress().host() + "\" does not have certificate.";
        }
    });

    tlsApp.onDisconnect([](net::socket::tls::SocketConnection* socketConnection) -> void {
        VLOG(0) << "Disconnect: " + socketConnection->getRemoteAddress().host() + ":" +
                       std::to_string(socketConnection->getRemoteAddress().port());
    });

    WebApp::start();

    return 0;
}
