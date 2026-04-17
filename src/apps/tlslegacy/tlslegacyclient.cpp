/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "TlsLegacySocketContext.h"
#include "core/SNodeC.h"
#include "log/Logger.h"
#include "net/in/stream/tls/SocketClient.h"

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    using SocketClient = net::in::stream::tls::SocketClient<apps::tlslegacy::TlsLegacyClientSocketContextFactory>;
    SocketClient client("tls-legacy-client");

    client.getConfig()->setCaCert("certs/Volker_Christian_-_Web_CA.crt");
    client.getConfig()->setCaCertAcceptUnknown(true);
    client.getConfig()->setNoCloseNotifyIsEOF(true);

    client.connect([instanceName = client.getConfig()->getInstanceName()](const SocketClient::SocketAddress& socketAddress,
                                                                          const core::socket::State& state) {
        if (state == core::socket::State::OK) {
            VLOG(1) << instanceName << ": connected to " << socketAddress.toString();
        } else if (state == core::socket::State::ERROR) {
            LOG(ERROR) << instanceName << ": " << socketAddress.toString() << ": " << state.what();
        }
    });

    return core::SNodeC::start();
}
