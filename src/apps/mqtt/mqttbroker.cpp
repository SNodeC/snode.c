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

#include "broker/SharedSocketContextFactory.h" // IWYU pragma: keep
#include "broker/SocketContextFactory.h"       // IWYU pragma: keep

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "config.h" // just for this example app
#include "core/SNodeC.h"
#include "log/Logger.h"
#include "net/in/stream/legacy/SocketServer.h"
#include "net/in/stream/tls/SocketServer.h"
#include "net/un/stream/legacy/SocketServer.h"

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using MQTTLegacyInServer = net::in::stream::legacy::SocketServer<mqtt::broker::SharedSocketContextFactory>;
    using LegacyInSocketConnection = MQTTLegacyInServer::SocketConnection;

    MQTTLegacyInServer mqttLegacyInServer(
        "legacyin",
        []([[maybe_unused]] LegacyInSocketConnection* socketConnection) -> void { // OnConnect
            VLOG(0) << "OnConnect";
        },
        []([[maybe_unused]] LegacyInSocketConnection* socketConnection) -> void { // OnConnected
            VLOG(0) << "OnConnected";
        },
        []([[maybe_unused]] LegacyInSocketConnection* socketConnection) -> void { // OnDisconnected
            VLOG(0) << "OnDisconnected";
        });

    mqttLegacyInServer.listen([](const MQTTLegacyInServer::SocketAddress& socketAddress, int errnum) -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << "mqttbroker listening on " << socketAddress.toString();
        }
    });

    using MQTTTLSInServer = net::in::stream::tls::SocketServer<mqtt::broker::SharedSocketContextFactory>;
    using TLSInSocketConnection = MQTTTLSInServer::SocketConnection;

    std::map<std::string, std::any> options{{"CertChain", SERVERCERTF}, {"CertChainKey", SERVERKEYF}, {"Password", KEYFPASS}};
    std::map<std::string, std::map<std::string, std::any>> sniCerts = {
        {"snodec.home.vchrist.at", {{"CertChain", SNODECCERTF}, {"CertChainKey", SERVERKEYF}, {"Password", KEYFPASS}}},
        {"www.vchrist.at", {{"CertChain", SNODECCERTF}, {"CertChainKey", SERVERKEYF}, {"Password", KEYFPASS}}}};

    MQTTTLSInServer mqttTLSInServer(
        "tlsin",
        []([[maybe_unused]] TLSInSocketConnection* socketConnection) -> void { // OnConnect
            VLOG(0) << "OnConnect";
        },
        []([[maybe_unused]] TLSInSocketConnection* socketConnection) -> void { // OnConnected
            VLOG(0) << "OnConnected";
        },
        []([[maybe_unused]] TLSInSocketConnection* socketConnection) -> void { // OnDisconnected
            VLOG(0) << "OnDisconnected";
        },
        options);

    mqttTLSInServer.addSniCerts(sniCerts);

    mqttTLSInServer.listen([](const MQTTTLSInServer::SocketAddress& socketAddress, int errnum) -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << "mqttbroker listening on " << socketAddress.toString();
        }
    });

    using MQTTLegacyUnServer = net::un::stream::legacy::SocketServer<mqtt::broker::SharedSocketContextFactory>;
    using LegacyUnSocketConnection = MQTTLegacyUnServer::SocketConnection;

    MQTTLegacyUnServer mqttLegacyUnServer(
        "legacyun",
        []([[maybe_unused]] LegacyUnSocketConnection* socketConnection) -> void { // OnConnect
            VLOG(0) << "OnConnect";
        },
        []([[maybe_unused]] LegacyUnSocketConnection* socketConnection) -> void { // OnConnected
            VLOG(0) << "OnConnected";
        },
        []([[maybe_unused]] LegacyUnSocketConnection* socketConnection) -> void { // OnDisconnected
            VLOG(0) << "OnDisconnected";
        });

    mqttLegacyUnServer.listen([](const LegacyUnSocketConnection::SocketAddress& socketAddress, int errnum) -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << "mqttbroker listening on " << socketAddress.toString();
        }
    });

    return core::SNodeC::start();
}
