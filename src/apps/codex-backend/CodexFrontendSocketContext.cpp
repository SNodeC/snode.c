/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend/CodexFrontendSocketContext.h"

#include "ai/openai/codex/frontend/Codec.h"
#include "ai/openai/codex/frontend/Messages.h"

#include <array>
#include <cstddef>
#include <exception>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace apps::codex_backend {

    using ai::openai::codex::frontend::CodecError;
    using ai::openai::codex::frontend::ErrorCode;
    using ai::openai::codex::frontend::FrontendConnectionCallbacks;
    using ai::openai::codex::frontend::OutboundMessage;

    struct CodexFrontendSocketContext::Lifetime {
        CodexFrontendSocketContext* context = nullptr;
    };

    CodexFrontendSocketContext::CodexFrontendSocketContext(core::socket::stream::SocketConnection* socketConnection,
                                                           ai::openai::codex::frontend::BackendAdapter& adapter,
                                                           SocketFrontendOptions options)
        : core::socket::stream::SocketContext(socketConnection)
        , adapter(adapter)
        , options(options)
        , framer(options.maximumFrameSize)
        , lifetime(std::make_shared<Lifetime>()) {
        lifetime->context = this;
    }

    void CodexFrontendSocketContext::onConnected() {
        try {
            const std::weak_ptr<Lifetime> weakLifetime = lifetime;
            frontendConnection =
                adapter.openConnection(FrontendConnectionCallbacks{[weakLifetime](const OutboundMessage& message) {
                                                                       const std::shared_ptr<Lifetime> locked = weakLifetime.lock();
                                                                       return locked && locked->context && locked->context->send(message);
                                                                   },
                                                                   [weakLifetime](const std::string& reason) {
                                                                       const std::shared_ptr<Lifetime> locked = weakLifetime.lock();
                                                                       if (locked && locked->context) {
                                                                           locked->context->adapterClosed(reason);
                                                                       }
                                                                   }});
        } catch (const std::exception& exception) {
            rejectFrame(ErrorCode::InternalError, std::string("failed to open frontend connection: ") + exception.what());
        } catch (...) {
            rejectFrame(ErrorCode::InternalError, "failed to open frontend connection");
        }
    }

    void CodexFrontendSocketContext::onDisconnected() {
        inputBlocked = true;
        disconnecting = true;
        if (lifetime) {
            lifetime->context = nullptr;
        }
        frontendConnection.close("Unix frontend disconnected");
        framer.clear();
        lifetime.reset();
    }

    std::size_t CodexFrontendSocketContext::onReceivedFromPeer() {
        std::array<char, 16 * 1024> bytes{};
        const std::size_t size = readFromPeer(bytes.data(), bytes.size());
        if (size == 0 || disconnecting || inputBlocked) {
            return size;
        }

        try {
            const JsonLineFramer::Result frameResult = framer.push(std::string_view(bytes.data(), size), [this](std::string frame) {
                if (disconnecting || inputBlocked) {
                    return;
                }
                const auto receiveResult = frontendConnection.receive(std::string_view(frame));
                if (receiveResult.status == ai::openai::codex::frontend::ConnectionReceiveStatus::Closing) {
                    inputBlocked = true;
                } else if (receiveResult.status == ai::openai::codex::frontend::ConnectionReceiveStatus::Closed) {
                    inputBlocked = true;
                    adapterClosed("frontend connection already closed");
                }
            });
            if (frameResult == JsonLineFramer::Result::FrameTooLarge) {
                rejectFrame(ErrorCode::FrameTooLarge, "frontend JSONL frame exceeds the configured maximum size");
            }
        } catch (const std::exception& exception) {
            rejectFrame(ErrorCode::InternalError, std::string("frontend frame handler failed: ") + exception.what());
        } catch (...) {
            rejectFrame(ErrorCode::InternalError, "frontend frame handler failed");
        }

        return size;
    }

    bool CodexFrontendSocketContext::onSignal([[maybe_unused]] int signum) {
        return true;
    }

    bool CodexFrontendSocketContext::send(const OutboundMessage& message) noexcept {
        if (disconnecting || message.compactJson.size() == std::numeric_limits<std::size_t>::max()) {
            return false;
        }

        try {
            const std::size_t totalQueued = getTotalQueued();
            const std::size_t totalSent = getTotalSent();
            if (totalQueued < totalSent) {
                return false;
            }
            const std::size_t outstanding = totalQueued - totalSent;
            const std::size_t frameSize = message.compactJson.size() + 1;
            if (frameSize > options.maximumOutboundBytes || outstanding > options.maximumOutboundBytes - frameSize) {
                return false;
            }

            std::string frame = message.compactJson;
            frame.push_back('\n');
            sendToPeer(frame.data(), frame.size());
            return true;
        } catch (...) {
            return false;
        }
    }

    void CodexFrontendSocketContext::adapterClosed([[maybe_unused]] const std::string& reason) noexcept {
        if (disconnecting) {
            return;
        }
        inputBlocked = true;
        disconnecting = true;
        try {
            shutdownRead();
            shutdownWrite();
        } catch (...) {
            try {
                close();
            } catch (...) {
            }
        }
    }

    void CodexFrontendSocketContext::rejectFrame(ErrorCode code, std::string message) noexcept {
        if (disconnecting) {
            return;
        }
        CodecError error;
        error.code = code;
        error.message = std::move(message);
        error.closeConnection = true;
        const auto result = frontendConnection.receiveError(std::move(error));
        if (result.status == ai::openai::codex::frontend::ConnectionReceiveStatus::Closing) {
            inputBlocked = true;
        } else if (result.status == ai::openai::codex::frontend::ConnectionReceiveStatus::Closed) {
            adapterClosed("frontend protocol error");
        }
    }

} // namespace apps::codex_backend
