/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend-client/ClientConnection.h"

#include "apps/codex-backend-client/CodexBackendClientSocketContext.h"

#include <utility>

namespace apps::codex_backend_client {

    ClientConnection::ClientConnection(ClientConnectionCallbacks callbacks)
        : callbacks(std::move(callbacks)) {
    }

    ClientConnection::~ClientConnection() {
        disconnect();
    }

    bool ClientConnection::send(const ai::openai::codex::frontend::ClientMessage& message) noexcept {
        return online && context != nullptr && context->send(message);
    }

    void ClientConnection::disconnect() noexcept {
        if (context != nullptr) {
            context->disconnect();
        }
    }

    bool ClientConnection::connected() const noexcept {
        return online && context != nullptr;
    }

    void ClientConnection::attach(CodexBackendClientSocketContext& context) noexcept {
        this->context = &context;
        online = false;
    }

    void ClientConnection::didConnect(CodexBackendClientSocketContext& context) noexcept {
        if (this->context != &context) {
            return;
        }
        online = true;
        try {
            if (callbacks.onConnected) {
                callbacks.onConnected();
            }
        } catch (...) {
        }
    }

    void ClientConnection::didReceive(CodexBackendClientSocketContext& context,
                                      const ai::openai::codex::frontend::ServerMessage& message) noexcept {
        if (this->context != &context) {
            return;
        }
        try {
            if (callbacks.onMessage) {
                callbacks.onMessage(message);
            }
        } catch (...) {
        }
    }

    void ClientConnection::didReceiveError(CodexBackendClientSocketContext& context,
                                           const ai::openai::codex::frontend::CodecError& error) noexcept {
        if (this->context != &context) {
            return;
        }
        try {
            if (callbacks.onProtocolError) {
                callbacks.onProtocolError(error);
            }
        } catch (...) {
        }
    }

    void ClientConnection::detach(CodexBackendClientSocketContext& context) noexcept {
        if (this->context != &context) {
            return;
        }
        const bool notify = online;
        online = false;
        this->context = nullptr;
        try {
            if (notify && callbacks.onDisconnected) {
                callbacks.onDisconnected();
            }
        } catch (...) {
        }
    }

} // namespace apps::codex_backend_client
