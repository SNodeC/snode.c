/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "core/SNodeC.h"
#include "log/Logger.h"
#include "model/servers.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using SocketServer = apps::echo::model::STREAM::EchoSocketServer;
    SocketServer server = apps::echo::model::STREAM::getServer();

#if (STREAM_TYPE == TLS)
    std::string cert = "/home/voc/projects/snodec/snode.c/certs/snode.c_-_server.pem";
    std::string key = "/home/voc/projects/snodec/snode.c/certs/Volker_Christian_-_Web_-_snode.c_-_server.key.encrypted.pem";
    std::string pass = "snode.c";

    std::map<std::string, std::map<std::string, std::variant<std::string, bool, ssl_option_t>>> sniCerts = {
        {"snodec.home.vchrist.at", {{"CertChain", cert}, {"CertKey", key}, {"CertKeyPassword", pass}}},
        {"www.vchrist.at", {{"CertChain", cert}, {"CertKey", key}, {"CertKeyPassword", pass}}}};

    server.addSniCerts(sniCerts);
#endif

    server.listen([](const SocketServer::SocketAddress& socketAddress, int errnum) -> void {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << "snode.c listening on " << socketAddress.toString();
        }
    });

    return core::SNodeC::start();
}

/*
#if (NET_TYPE == IN) // in
#if (STREAM_TYPE == LEGACY)
    server.listen(8080, 5, [](const SocketServer::Socket& socket, int errnum) -> void {
#elif (STREAM_TYPE == TLS)
    server.listen(8088, 5, [](const SocketServer::Socket& socket, int errnum) -> void {
#endif
#elif (NET_TYPE == IN6) // in6
#if (STREAM_TYPE == LEGACY)
    server.listen(8080, 5, [](const SocketServer::Socket& socket, int errnum) -> void {
#elif (STREAM_TYPE == TLS)
     server.listen(8088, 5, [](const SocketServer::Socket& socket, int errnum) -> void {
#endif
#elif (NET_TYPE == L2) //
    // ATLAS: 10:3D:1C:AC:BA:9C
    // TITAN: A4:B1:C1:2C:82:37
    // USB: 44:01:BB:A3:63:32

    // server.listen("A4:B1:C1:2C:82:37", 0x1023, 5, [](const SocketServer::Socket& socket, int errnum) -> void { // titan
     server.listen("10:3D:1C:AC:BA:9C", 0x1023, 5, [](const SocketServer::Socket& socket, int errnum) -> void { // titan
#elif (NET_TYPE == RC) // rf
    // server.listen("A4:B1:C1:2C:82:37", 1, 5, [](const SocketServer::Socket& socket, int errnum) -> void { // titan
     server.listen("10:3D:1C:AC:BA:9C", 1, 5, [](const SocketServer::Socket& socket, int errnum) -> void { // titan
#elif (NET_TYPE == UN) // un
     server.listen("/tmp/testme", 5, [](const SocketServer::Socket& socket, int errnum) -> void { // titan
#endif
        if (errnum != 0) {
            PLOG(FATAL) << "listen";
        } else {
            VLOG(0) << "snode.c listening on " << socket.getBindAddress().toString();
        }

#ifdef NET_TYPE
    });
#endif
*/
