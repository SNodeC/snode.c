/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef APPS_CODEX_BACKEND_CLIENT_CODEXBACKENDCLIENTSOCKETCONTEXT_H
#define APPS_CODEX_BACKEND_CLIENT_CODEXBACKENDCLIENTSOCKETCONTEXT_H

#include "ai/openai/codex/frontend/Messages.h"
#include "apps/codex-backend-client/JsonLineFramer.h"
#include "core/socket/stream/SocketContext.h"

#include <cstddef>
#include <string>

namespace ai::openai::codex::frontend {
    struct CodecError;
}

namespace core::socket::stream {
    class SocketConnection;
}

namespace apps::codex_backend_client {

    class ClientConnection;

    class CodexBackendClientSocketContext final : public core::socket::stream::SocketContext {
    public:
        CodexBackendClientSocketContext(core::socket::stream::SocketConnection* socketConnection,
                                        ClientConnection& connection,
                                        std::size_t maximumFrameSize);

    private:
        friend class ClientConnection;

        void onConnected() override;
        void onDisconnected() override;
        std::size_t onReceivedFromPeer() override;
        bool onSignal(int signum) override;

        [[nodiscard]] bool send(const ai::openai::codex::frontend::ClientMessage& message) noexcept;
        void disconnect() noexcept;
        void handleFrame(std::string frame) noexcept;
        void fail(ai::openai::codex::frontend::CodecError error) noexcept;

        ClientConnection& connection;
        JsonLineFramer framer;
        bool disconnecting = false;
    };

} // namespace apps::codex_backend_client

#endif // APPS_CODEX_BACKEND_CLIENT_CODEXBACKENDCLIENTSOCKETCONTEXT_H
