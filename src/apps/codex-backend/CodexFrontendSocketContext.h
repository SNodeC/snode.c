/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef APPS_CODEX_BACKEND_CODEXFRONTENDSOCKETCONTEXT_H
#define APPS_CODEX_BACKEND_CODEXFRONTENDSOCKETCONTEXT_H

#include "ai/openai/codex/frontend/BackendAdapter.h"
#include "apps/codex-backend/Configuration.h"
#include "apps/codex-backend/JsonLineFramer.h"
#include "core/socket/stream/SocketContext.h"

#include <cstddef>
#include <memory>
#include <string>

namespace ai::openai::codex::frontend {
    enum class ErrorCode;
}

namespace core::socket::stream {
    class SocketConnection;
}

namespace apps::codex_backend {

    class CodexFrontendSocketContext final : public core::socket::stream::SocketContext {
    public:
        CodexFrontendSocketContext(core::socket::stream::SocketConnection* socketConnection,
                                   ai::openai::codex::frontend::BackendAdapter& adapter,
                                   SocketFrontendOptions options);

    private:
        struct Lifetime;

        void onConnected() override;
        void onDisconnected() override;
        std::size_t onReceivedFromPeer() override;
        bool onSignal(int signum) override;

        bool send(const ai::openai::codex::frontend::OutboundMessage& message) noexcept;
        void adapterClosed(const std::string& reason) noexcept;
        void rejectFrame(ai::openai::codex::frontend::ErrorCode code, std::string message) noexcept;

        ai::openai::codex::frontend::BackendAdapter& adapter;
        SocketFrontendOptions options;
        JsonLineFramer framer;
        ai::openai::codex::frontend::FrontendConnection frontendConnection;
        std::shared_ptr<Lifetime> lifetime;
        bool inputBlocked = false;
        bool disconnecting = false;
    };

} // namespace apps::codex_backend

#endif // APPS_CODEX_BACKEND_CODEXFRONTENDSOCKETCONTEXT_H
