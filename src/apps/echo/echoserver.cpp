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

#include "core/SNodeC.h"
#include "model/servers.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>

#if (STREAM_TYPE == TLS)

#include <map>
#include <utility>
#include <variant>

#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

// IWYU pragma: no_include "core/socket/stream/tls/ssl_utils.h"

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using SocketServer = apps::echo::model::STREAM::EchoSocketServer;
    const SocketServer server = apps::echo::model::STREAM::getServer();

#if (STREAM_TYPE == TLS)
    std::string cert = "/home/voc/projects/snodec/snode.c/certs/snode.c_-_server.pem";
    std::string key = "/home/voc/projects/snodec/snode.c/certs/Volker_Christian_-_Web_-_snode.c_-_server.key.encrypted.pem";
    std::string pass = "snode.c";

    std::map<std::string, std::map<std::string, std::variant<std::string, bool, ssl_option_t>>> sniCerts = {
        {"snodec.home.vchrist.at", {{"Cert", cert}, {"CertKey", key}, {"CertKeyPassword", pass}}},
        {"www.vchrist.at", {{"Cert", cert}, {"CertKey", key}, {"CertKeyPassword", pass}}}};

    server.getConfig().addSniCerts(sniCerts);
#endif

    //    server.getConfig().setDisabled();
    //    server.getConfig().setDisabled(false);

    server.listen([instanceName = server.getConfig().getInstanceName()](const SocketServer::SocketAddress& socketAddress,
                                                                        const core::socket::State& state) {
        switch (state) {
            case core::socket::State::OK:
                VLOG(1) << instanceName << ": listening on '" << socketAddress.toString() << "'";
                break;
            case core::socket::State::DISABLED:
                VLOG(1) << instanceName << ": disabled";
                break;
            case core::socket::State::ERROR:
                LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                break;
            case core::socket::State::FATAL:
                LOG(FATAL) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
                break;
        }
    });

    return core::SNodeC::start();
}

/*
#if (NET_TYPE == IN) // in
#if (STREAM_TYPE == LEGACY)
    server.listen(8080, 5, [](const SocketServer::Socket& socket, int errnum) {
#elif (STREAM_TYPE == TLS)
    server.listen(8088, 5, [](const SocketServer::Socket& socket, int errnum) {
#endif
#elif (NET_TYPE == IN6) // in6
#if (STREAM_TYPE == LEGACY)
    server.listen(8080, 5, [](const SocketServer::Socket& socket, int errnum) {
#elif (STREAM_TYPE == TLS)
     server.listen(8088, 5, [](const SocketServer::Socket& socket, int errnum) {
#endif
#elif (NET_TYPE == L2) //
    // ATLAS: 10:3D:1C:AC:BA:9C
    // TITAN: A4:B1:C1:2C:82:37
    // USB: 44:01:BB:A3:63:32

    // server.listen("A4:B1:C1:2C:82:37", 0x1023, 5, [](const SocketServer::Socket& socket, int errnum) { // titan
     server.listen("10:3D:1C:AC:BA:9C", 0x1023, 5, [](const SocketServer::Socket& socket, int errnum) { // titan
#elif (NET_TYPE == RC) // rf
    // server.listen("A4:B1:C1:2C:82:37", 1, 5, [](const SocketServer::Socket& socket, int errnum) { // titan
     server.listen("10:3D:1C:AC:BA:9C", 1, 5, [](const SocketServer::Socket& socket, int errnum) { // titan
#elif (NET_TYPE == UN) // un
     server.listen("/tmp/testme", 5, [](const SocketServer::Socket& socket, int errnum) { // titan
#endif
        if (errnum != 0) {
            PLOG(FATAL) << "listen";
        } else {
            VLOG(1) << "snode.c listening on " << socket.getBindAddress().toString();
        }

#ifdef NET_TYPE
    });
#endif
*/
