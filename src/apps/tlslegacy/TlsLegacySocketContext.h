/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#ifndef APPS_TLSLEGACY_TLSLEGACYSOCKETCONTEXT_H
#define APPS_TLSLEGACY_TLSLEGACYSOCKETCONTEXT_H

#include "core/socket/stream/SocketContext.h"
#include "core/socket/stream/SocketContextFactory.h"
#include "core/timer/Timer.h"

#include <string>

namespace apps::tlslegacy {

    class TlsLegacySocketContext : public core::socket::stream::SocketContext {
    public:
        enum class Role { SERVER, CLIENT };

        explicit TlsLegacySocketContext(core::socket::stream::SocketConnection* socketConnection, Role role);

    private:
        void onConnected() override;
        void onDisconnected() override;
        bool onSignal(int signum) override;
        std::size_t onReceivedFromPeer() override;

        void onClientLine(const std::string& line);
        void onServerLine(const std::string& line);
        void startLegacyRetryTimer(const std::string& payload);

        Role role;
        std::string inboundBuffer;

        bool tlsReplySeen = false;
        bool legacyReplySeen = false;
        bool legacyPayloadSeen = false;

        core::timer::Timer legacyRetryTimer;
    };

    class TlsLegacyServerSocketContextFactory : public core::socket::stream::SocketContextFactory {
    private:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override;
    };

    class TlsLegacyClientSocketContextFactory : public core::socket::stream::SocketContextFactory {
    private:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override;
    };

} // namespace apps::tlslegacy

#endif
