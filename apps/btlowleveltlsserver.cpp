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

#include "config.h"                                       // for CLIENTCAFILE
#include "log/Logger.h"                                   // for Writer
#include "net/SNodeC.h"                                   // for SNodeC
#include "net/socket/bluetooth/address/RfCommAddress.h"   // for RfCommAddress
#include "net/socket/bluetooth/rfcomm/tls/SocketServer.h" // for SocketServer
#include "net/socket/stream/SocketContext.h"              // for SocketProt...
#include "net/socket/stream/SocketContextFactory.h"       // for SocketProt...
#include "net/socket/stream/SocketServer.h"               // for SocketServ...

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

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace net::socket::bluetooth::rfcomm::tls;

class SimpleSocketProtocol : public net::socket::stream::SocketContext {
public:
    void onReceiveFromPeer(const char* junk, std::size_t junkLen) override {
        VLOG(0) << "Data to reflect: " << std::string(junk, junkLen);
        sendToPeer(junk, junkLen);
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
    net::socket::stream::SocketContext* create() const override {
        return new SimpleSocketProtocol();
    }
};

int main(int argc, char* argv[]) {
    net::SNodeC::init(argc, argv);

    SocketServer<SimpleSocketProtocolFactory> btServer(
        [](const SocketServer<SimpleSocketProtocolFactory>::SocketAddress& localAddress,
           const SocketServer<SimpleSocketProtocolFactory>::SocketAddress& remoteAddress) -> void { // OnConnect
            VLOG(0) << "OnConnect";

            VLOG(0) << "\tServer: " + localAddress.toString();
            VLOG(0) << "\tClient: " + remoteAddress.toString();
        },
        [](SocketServer<SimpleSocketProtocolFactory>::SocketConnection* socketConnection) -> void { // onConnected
            VLOG(0) << "OnConnected";

            X509* client_cert = SSL_get_peer_certificate(socketConnection->getSSL());

            if (client_cert != NULL) {
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
                    static_cast<GENERAL_NAMES*>(X509_get_ext_d2i(client_cert, NID_subject_alt_name, NULL, NULL));

                int32_t altNameCount = sk_GENERAL_NAME_num(subjectAltNames);
                VLOG(2) << "\t   Subject alternative name count: " << altNameCount;
                for (int32_t i = 0; i < altNameCount; ++i) {
                    GENERAL_NAME* generalName = sk_GENERAL_NAME_value(subjectAltNames, i);
                    if (generalName->type == GEN_URI) {
                        std::string subjectAltName =
                            std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.uniformResourceIdentifier)),
                                        ASN1_STRING_length(generalName->d.uniformResourceIdentifier));
                        VLOG(2) << "\t      SAN (URI): '" + subjectAltName;
                    } else if (generalName->type == GEN_DNS) {
                        std::string subjectAltName =
                            std::string(reinterpret_cast<const char*>(ASN1_STRING_get0_data(generalName->d.dNSName)),
                                        ASN1_STRING_length(generalName->d.dNSName));
                        VLOG(2) << "\t      SAN (DNS): '" + subjectAltName;
                    } else {
                        VLOG(2) << "\t      SAN (Type): '" + std::to_string(generalName->type);
                    }
                }
                sk_GENERAL_NAME_pop_free(subjectAltNames, GENERAL_NAME_free);

                X509_free(client_cert);
            } else {
                VLOG(2) << "\tClient certificate: no certificate";
            }
        },
        [](SocketServer<SimpleSocketProtocolFactory>::SocketConnection* socketConnection) -> void { // onDisconnect
            VLOG(0) << "OnDisconnect";

            VLOG(0) << "\tServer: " + socketConnection->getLocalAddress().toString();
            VLOG(0) << "\tClient: " + socketConnection->getRemoteAddress().toString();
        },
        {{"certChain", SERVERCERTF}, {"keyPEM", SERVERKEYF}, {"password", KEYFPASS}, {"caFile", CLIENTCAFILE}});

    btServer.listen(SocketServer<SimpleSocketProtocolFactory>::SocketAddress("A4:B1:C1:2C:82:37", 1), 5, [](int errnum) -> void { // titan
        if (errnum != 0) {
            PLOG(ERROR) << "BT listen: " << errnum;
        } else {
            LOG(INFO) << "BT listening on channel 1";
        }
    });

    return net::SNodeC::start();
}
