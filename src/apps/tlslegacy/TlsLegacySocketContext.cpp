/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "TlsLegacySocketContext.h"

#include "SemanticLog.h"
#include "core/socket/stream/SocketConnection.h"
#include "log/Logger.h"

#include <functional>

namespace {
    constexpr const char* TLS_HELLO = "TLS_HELLO\n";
    constexpr const char* TLS_ACK = "TLS_ACK\n";
    constexpr const char* LEGACY_HELLO = "LEGACY_HELLO\n";
    constexpr const char* LEGACY_ACK = "LEGACY_ACK\n";
} // namespace

namespace apps::tlslegacy {

    TlsLegacySocketContext::TlsLegacySocketContext(core::socket::stream::SocketConnection* socketConnection, Role role)
        : core::socket::stream::SocketContext(socketConnection)
        , role(role) {
    }

    void TlsLegacySocketContext::onConnected() {
        snode::semantic::appLog().info() << getSocketConnection()->getConnectionName() << ": connected";

        if (role == Role::CLIENT) {
            sendToPeer(TLS_HELLO);
            snode::semantic::appLog().debug() << getSocketConnection()->getConnectionName() << ": sent TLS greeting";
        }
    }

    void TlsLegacySocketContext::onDisconnected() {
        legacyRetryTimer.cancel();
        snode::semantic::appLog().debug() << getSocketConnection()->getConnectionName() << ": disconnected";
    }

    bool TlsLegacySocketContext::onSignal([[maybe_unused]] int signum) {
        return true;
    }

    void TlsLegacySocketContext::startLegacyRetryTimer(const std::string& payload) {
        legacyRetryTimer.cancel();

        legacyRetryTimer = core::timer::Timer::intervalTimer(
            [this, payload](const std::function<void()>& stop) {
                if (role == Role::CLIENT && legacyReplySeen) {
                    stop();
                    return;
                }

                sendToPeer(payload);
                snode::semantic::appLog().debug()
                    << getSocketConnection()->getConnectionName() << ": trying post-TLS legacy payload: " << payload;
            },
            utils::Timeval(0.5));
    }

    void TlsLegacySocketContext::onClientLine(const std::string& line) {
        if (line == TLS_ACK && !tlsReplySeen) {
            tlsReplySeen = true;
            snode::semantic::appLog().debug() << getSocketConnection()->getConnectionName()
                                              << ": got TLS ack, initiating TLS shutdown handshake (close_notify) " << line;
            shutdownWrite();
            startLegacyRetryTimer(LEGACY_HELLO);
        } else if (line == LEGACY_ACK && !legacyReplySeen) {
            legacyReplySeen = true;
            legacyRetryTimer.cancel();
            snode::semantic::appLog().debug() << getSocketConnection()->getConnectionName()
                                              << ": got LEGACY ack -> post-TLS plaintext path works " << line;
            shutdownWrite();
        }
    }

    void TlsLegacySocketContext::onServerLine(const std::string& line) {
        if (line == TLS_HELLO && !tlsReplySeen) {
            tlsReplySeen = true;
            sendToPeer(TLS_ACK);
            snode::semantic::appLog().debug() << getSocketConnection()->getConnectionName()
                                              << ": TLS phase complete, waiting for peer close_notify " << line;
        } else if (line == LEGACY_HELLO && !legacyPayloadSeen) {
            legacyPayloadSeen = true;
            legacyRetryTimer.cancel();
            sendToPeer(LEGACY_ACK);
            snode::semantic::appLog().debug() << getSocketConnection()->getConnectionName()
                                              << ": received LEGACY payload after TLS shutdown " << line;
            shutdownWrite();
        }
    }

    std::size_t TlsLegacySocketContext::onReceivedFromPeer() {
        char chunk[4096];
        const std::size_t chunkLen = readFromPeer(chunk, sizeof(chunk));

        if (chunkLen == 0) {
            return 0;
        }

        inboundBuffer.append(chunk, chunkLen);

        std::size_t pos = std::string::npos;
        while ((pos = inboundBuffer.find('\n')) != std::string::npos) {
            const std::string line = inboundBuffer.substr(0, pos + 1);
            inboundBuffer.erase(0, pos + 1);

            if (role == Role::CLIENT) {
                onClientLine(line);
            } else {
                onServerLine(line);
            }
        }

        return chunkLen;
    }

    core::socket::stream::SocketContext*
    TlsLegacyServerSocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        return new TlsLegacySocketContext(socketConnection, TlsLegacySocketContext::Role::SERVER);
    }

    core::socket::stream::SocketContext*
    TlsLegacyClientSocketContextFactory::create(core::socket::stream::SocketConnection* socketConnection) {
        return new TlsLegacySocketContext(socketConnection, TlsLegacySocketContext::Role::CLIENT);
    }

} // namespace apps::tlslegacy
