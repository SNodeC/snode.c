/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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

#include "config.h"                                       // for CLIENTCERTF
#include "log/Logger.h"                                   // for Writer
#include "net/SNodeC.h"                                   // for SNodeC
#include "net/socket/bluetooth/address/RfCommAddress.h"   // for RfCommAddress
#include "net/socket/bluetooth/rfcomm/tls/SocketClient.h" // for SocketClient
#include "net/socket/stream/SocketClient.h"               // for SocketClie...
#include "net/socket/stream/SocketContext.h"              // for SocketProt...
#include "net/socket/stream/SocketContextFactory.h"       // for SocketProt...

namespace net::socket::stream {
    class SocketConnection;
} // namespace net::socket::stream

#include <any>                // for any
#include <cstddef>            // for NULL, size_t
#include <functional>         // for function
#include <openssl/asn1.h>     // for ASN1_STRIN...
#include <openssl/crypto.h>   // for OPENSSL_free
#include <openssl/obj_mac.h>  // for NID_subjec...
#include <openssl/ossl_typ.h> // for X509
#include <openssl/ssl3.h>     // for SSL_get_pe...
#include <openssl/x509.h>     // for X509_NAME_...
#include <openssl/x509v3.h>   // for GENERAL_NAME
#include <stdint.h>           // for int32_t
#include <string>             // for string
#include <sys/types.h>        // for ssize_t

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace net::socket::bluetooth::rfcomm::tls;

class SimpleSocketProtocol : public net::socket::stream::SocketContext {
public:
    explicit SimpleSocketProtocol(net::socket::stream::SocketConnection* socketConnection)
        : net::socket::stream::SocketContext(socketConnection) {
    }

    void onReceiveFromPeer() override {
        char junk[4096];

        ssize_t ret = readFromPeer(junk, 4096);

        if (ret > 0) {
            std::size_t count = static_cast<std::size_t>(ret);
            VLOG(0) << "Data to reflect: " << std::string(junk, count);
            sendToPeer(junk, count);
        }
    }

    void onWriteError(int errnum) override {
        VLOG(0) << "OnWriteError: " << errnum;
    }

    void onReadError(int errnum) override {
        VLOG(0) << "OnReadError: " << errnum;
    }
};

class SimpleSocketProtocolFactory : public net::socket::stream::SocketContextFactory {
private:
    net::socket::stream::SocketContext* create(net::socket::stream::SocketConnection* socketConnection) override {
        return new SimpleSocketProtocol(socketConnection);
    }
};

SocketClient<SimpleSocketProtocolFactory> getClient() {
    SocketClient<SimpleSocketProtocolFactory> client(
        [](const SocketClient<SimpleSocketProtocolFactory>::SocketAddress& localAddress,
           const SocketClient<SimpleSocketProtocolFactory>::SocketAddress& remoteAddress) -> void { // OnConnect
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tServer: " + remoteAddress.toString();
            VLOG(0) << "\tClient: " + localAddress.toString();
        },
        [](SocketClient<SimpleSocketProtocolFactory>::SocketConnection* socketConnection) -> void { // onConnected
            VLOG(0) << "OnConnected";

            X509* server_cert = SSL_get_peer_certificate(socketConnection->getSSL());
            if (server_cert != NULL) {
                long verifyErr = SSL_get_verify_result(socketConnection->getSSL());

                VLOG(0) << "     Server certificate: " + std::string(X509_verify_cert_error_string(verifyErr));

                char* str = X509_NAME_oneline(X509_get_subject_name(server_cert), 0, 0);
                VLOG(0) << "        Subject: " + std::string(str);
                OPENSSL_free(str);

                str = X509_NAME_oneline(X509_get_issuer_name(server_cert), 0, 0);
                VLOG(0) << "        Issuer: " + std::string(str);
                OPENSSL_free(str);

                // We could do all sorts of certificate verification stuff here before deallocating the certificate.

                GENERAL_NAMES* subjectAltNames =
                    static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(server_cert, NID_subject_alt_name, NULL, NULL));

                int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);
                VLOG(0) << "        Subject alternative name count: " << altNameCount;
                for (int32_t i = 0; i < altNameCount; ++i) {
                    GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
                    if (generalName->type == GEN_URI) {
                        std::string subjectAltName =
                            std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),
                                        static_cast<std::size_t>(ASN1_STRING_length(generalName->d.uniformResourceIdentifier)));
                        VLOG(0) << "           SAN (URI): '" + subjectAltName;
                    } else if (generalName->type == GEN_DNS) {
                        std::string subjectAltName =
                            std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                        static_cast<std::size_t>(ASN1_STRING_length(generalName->d.dNSName)));
                        VLOG(0) << "           SAN (DNS): '" + subjectAltName;
                    } else {
                        VLOG(0) << "           SAN (Type): '" + std::to_string(generalName->type);
                    }
                }
                sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);

                X509_free(server_cert);
            } else {
                VLOG(0) << "     Server certificate: no certificate";
            }

            socketConnection->sendToPeer("Hello rfcomm connection!");
        },
        [](SocketClient<SimpleSocketProtocolFactory>::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";

            VLOG(0) << "\tServer: " + socketConnection->getRemoteAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getLocalAddress().toString();
        },
        {{"certChain", CLIENTCERTF}, {"keyPEM", CLIENTKEYF}, {"password", KEYFPASS}, {"caFile", SERVERCAFILE}});

    return client;
}

int main(int argc, char* argv[]) {
    net::SNodeC::init(argc, argv);

    SocketClient<SimpleSocketProtocolFactory>::SocketAddress remoteAddress("A4:B1:C1:2C:82:37", 1); // titan
    SocketClient<SimpleSocketProtocolFactory>::SocketAddress bindAddress("44:01:BB:A3:63:32");      // mpow

    SocketClient client = getClient();

    client.connect(remoteAddress, bindAddress, [](int err) -> void {
        if (err) {
            PLOG(ERROR) << "Connect: " << std::to_string(err);
        } else {
            VLOG(0) << "Connected";
        }
    });

    return net::SNodeC::start();
}
