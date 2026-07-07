/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "SemanticLog.h"
#include "TlsLegacySocketContext.h"
#include "core/SNodeC.h"
#include "log/Logger.h"
#include "net/in/stream/tls/SocketServer.h"

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using SocketServer = net::in::stream::tls::SocketServer<apps::tlslegacy::TlsLegacyServerSocketContextFactory>;
    const SocketServer server("tls-legacy-server");

    server.getConfig()->setCaCert("/home/voc/projects/snodec/snode.c/certs/Volker_Christian_-_Web_CA.crt");

    server.getConfig()->setCert("/home/voc/projects/snodec/snode.c/certs/snodec.home.vchrist.at_-_snode.c_-_server.pem");
    server.getConfig()->setCertKey("/home/voc/projects/snodec/snode.c/certs/Volker_Christian_-_Web_-_snode.c_-_server.key.encrypted.pem");
    server.getConfig()->setCertKeyPassword("snode.c");

    server.getConfig()->setCaCertAcceptUnknown(false);
    server.getConfig()->setNoCloseNotifyIsEOF(true); // switch back to unencrypted exchange in case tls shutdown was successful

    server.listen([instanceName = server.getConfig()->getInstanceName()](const SocketServer::SocketAddress& socketAddress,
                                                                         const core::socket::State& state) {
        if (state == core::socket::State::OK) {
            snode::semantic::appLog().info() << instanceName << ": listening on " << socketAddress.toString();
        } else if (state == core::socket::State::ERROR) {
            snode::semantic::appLog().error() << instanceName << ": " << socketAddress.toString() << ": " << state.what();
        }
    });

    return core::SNodeC::start();
}
