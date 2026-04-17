/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "TlsLegacySocketContext.h"
#include "core/SNodeC.h"
#include "log/Logger.h"
#include "net/in/stream/tls/SocketServer.h"

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using SocketServer = net::in::stream::tls::SocketServer<apps::tlslegacy::TlsLegacyServerSocketContextFactory>;
    SocketServer server("tls-legacy-server");

    server.getConfig()->setCert("certs/snodec.home.vchrist.at_-_snode.c_-_server.pem");
    server.getConfig()->setCertKey("certs/Volker_Christian_-_Web_-_snode.c_-_server.key.encrypted.pem");
    server.getConfig()->setCertKeyPassword("snode.c");
    server.getConfig()->setNoCloseNotifyIsEOF(true);

    server.listen([instanceName = server.getConfig()->getInstanceName()](const SocketServer::SocketAddress& socketAddress,
                                                                         const core::socket::State& state) {
        if (state == core::socket::State::OK) {
            VLOG(1) << instanceName << ": listening on " << socketAddress.toString();
        } else if (state == core::socket::State::ERROR) {
            LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
        }
    });

    return core::SNodeC::start();
}
