/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend-client/CodexBackendClientSocketContext.h"

#include "ai/openai/codex/frontend/Codec.h"
#include "apps/codex-backend-client/ClientConnection.h"

#include <array>
#include <exception>
#include <nlohmann/json.hpp>
#include <optional>
#include <string_view>
#include <utility>
#include <variant>

namespace apps::codex_backend_client {

    namespace frontend = ai::openai::codex::frontend;

    CodexBackendClientSocketContext::CodexBackendClientSocketContext(core::socket::stream::SocketConnection* socketConnection,
                                                                     ClientConnection& connection,
                                                                     std::size_t maximumFrameSize)
        : core::socket::stream::SocketContext(socketConnection)
        , connection(connection)
        , framer(maximumFrameSize) {
        connection.attach(*this);
    }

    void CodexBackendClientSocketContext::onConnected() {
        const frontend::ClientMessage hello = frontend::Hello{std::nullopt, frontend::Json::object()};
        if (!send(hello)) {
            frontend::CodecError error;
            error.code = frontend::ErrorCode::InternalError;
            error.message = "failed to send the frontend hello";
            error.closeConnection = true;
            fail(std::move(error));
            return;
        }
        connection.didConnect(*this);
    }

    void CodexBackendClientSocketContext::onDisconnected() {
        disconnecting = true;
        framer.clear();
        connection.detach(*this);
    }

    std::size_t CodexBackendClientSocketContext::onReceivedFromPeer() {
        std::array<char, 16 * 1024> bytes{};
        const std::size_t size = readFromPeer(bytes.data(), bytes.size());
        if (size == 0 || disconnecting) {
            return size;
        }

        try {
            const JsonLineFramer::Result result = framer.push(std::string_view(bytes.data(), size), [this](std::string frame) {
                if (!disconnecting) {
                    handleFrame(std::move(frame));
                }
            });
            if (result == JsonLineFramer::Result::FrameTooLarge && !disconnecting) {
                frontend::CodecError error;
                error.code = frontend::ErrorCode::FrameTooLarge;
                error.message = "frontend server JSONL frame exceeds the configured maximum size";
                error.closeConnection = true;
                fail(std::move(error));
            }
        } catch (const std::exception& exception) {
            frontend::CodecError error;
            error.code = frontend::ErrorCode::InternalError;
            error.message = std::string("frontend frame handling failed: ") + exception.what();
            error.closeConnection = true;
            fail(std::move(error));
        } catch (...) {
            frontend::CodecError error;
            error.code = frontend::ErrorCode::InternalError;
            error.message = "frontend frame handling failed";
            error.closeConnection = true;
            fail(std::move(error));
        }
        return size;
    }

    bool CodexBackendClientSocketContext::onSignal([[maybe_unused]] int signum) {
        disconnect();
        return true;
    }

    bool CodexBackendClientSocketContext::send(const frontend::ClientMessage& message) noexcept {
        if (disconnecting) {
            return false;
        }
        try {
            const auto serialized = frontend::Codec::serializeClient(message);
            if (!serialized) {
                frontend::CodecError error = serialized.error();
                connection.didReceiveError(*this, error);
                return false;
            }
            std::string frame = serialized.value();
            frame.push_back('\n');
            sendToPeer(frame.data(), frame.size());
            return true;
        } catch (const std::exception& exception) {
            frontend::CodecError error;
            error.code = frontend::ErrorCode::InternalError;
            error.message = std::string("failed to send frontend message: ") + exception.what();
            connection.didReceiveError(*this, error);
        } catch (...) {
            frontend::CodecError error;
            error.code = frontend::ErrorCode::InternalError;
            error.message = "failed to send frontend message";
            connection.didReceiveError(*this, error);
        }
        return false;
    }

    void CodexBackendClientSocketContext::disconnect() noexcept {
        if (disconnecting) {
            return;
        }
        disconnecting = true;
        try {
            close();
        } catch (...) {
        }
    }

    void CodexBackendClientSocketContext::handleFrame(std::string frame) noexcept {
        auto decoded = frontend::Codec::decodeServer(std::string_view(frame));
        if (!decoded) {
            frontend::CodecError error = std::move(decoded).error();
            error.closeConnection = true;
            fail(std::move(error));
            return;
        }

        frontend::ServerMessage message = std::move(decoded).value();
        const bool closeConnection = std::holds_alternative<frontend::ProtocolErrorMessage>(message) &&
                                     std::get<frontend::ProtocolErrorMessage>(message).closeConnection;
        connection.didReceive(*this, message);
        if (closeConnection) {
            disconnect();
        }
    }

    void CodexBackendClientSocketContext::fail(frontend::CodecError error) noexcept {
        if (disconnecting) {
            return;
        }
        connection.didReceiveError(*this, error);
        disconnect();
    }

} // namespace apps::codex_backend_client
