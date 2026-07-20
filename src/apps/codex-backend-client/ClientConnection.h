/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef APPS_CODEX_BACKEND_CLIENT_CLIENTCONNECTION_H
#define APPS_CODEX_BACKEND_CLIENT_CLIENTCONNECTION_H

#include "ai/openai/codex/frontend/Messages.h"

#include <functional>

namespace ai::openai::codex::frontend {
    struct CodecError;
}

namespace apps::codex_backend_client {

    class CodexBackendClientSocketContext;

    struct ClientConnectionCallbacks {
        std::function<void()> onConnected;
        std::function<void(const ai::openai::codex::frontend::ServerMessage&)> onMessage;
        std::function<void(const ai::openai::codex::frontend::CodecError&)> onProtocolError;
        std::function<void()> onDisconnected;
    };

    // Stable application-side handle for a framework-owned socket context.
    // The context attaches and detaches itself as its connection comes and
    // goes, so callers never retain a pointer to a self-deleting context.
    class ClientConnection {
    public:
        explicit ClientConnection(ClientConnectionCallbacks callbacks = {});
        ClientConnection(const ClientConnection&) = delete;
        ClientConnection(ClientConnection&&) = delete;

        ClientConnection& operator=(const ClientConnection&) = delete;
        ClientConnection& operator=(ClientConnection&&) = delete;

        ~ClientConnection();

        [[nodiscard]] bool send(const ai::openai::codex::frontend::ClientMessage& message) noexcept;
        void disconnect() noexcept;

        [[nodiscard]] bool connected() const noexcept;

    private:
        friend class CodexBackendClientSocketContext;

        void attach(CodexBackendClientSocketContext& context) noexcept;
        void didConnect(CodexBackendClientSocketContext& context) noexcept;
        void didReceive(CodexBackendClientSocketContext& context, const ai::openai::codex::frontend::ServerMessage& message) noexcept;
        void didReceiveError(CodexBackendClientSocketContext& context, const ai::openai::codex::frontend::CodecError& error) noexcept;
        void detach(CodexBackendClientSocketContext& context) noexcept;

        ClientConnectionCallbacks callbacks;
        CodexBackendClientSocketContext* context = nullptr;
        bool online = false;
    };

} // namespace apps::codex_backend_client

#endif // APPS_CODEX_BACKEND_CLIENT_CLIENTCONNECTION_H
