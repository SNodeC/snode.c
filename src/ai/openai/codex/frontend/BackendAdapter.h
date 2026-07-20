/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_FRONTEND_BACKENDADAPTER_H
#define AI_OPENAI_CODEX_FRONTEND_BACKENDADAPTER_H

#include "ai/openai/codex/backend/BackendCore.h"
#include "ai/openai/codex/frontend/Codec.h"
#include "ai/openai/codex/frontend/EventCoalescer.h"
#include "ai/openai/codex/frontend/EventJournal.h"
#include "ai/openai/codex/frontend/Messages.h"
#include "ai/openai/codex/frontend/Protocol.h"
#include "ai/openai/codex/frontend/UpdateBatch.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace ai::openai::codex::frontend {

    struct OutboundMessage {
        ServerMessage message;
        std::string compactJson;
        std::size_t serializedBytes = 0;

        bool operator==(const OutboundMessage&) const = default;
    };

    struct FrontendConnectionCallbacks {
        // Return false when the transport cannot accept the message without
        // exceeding its own bounded queue. Only this frontend is then closed.
        using Send = std::function<bool(const OutboundMessage&)>;
        using Closed = std::function<void(const std::string&)>;

        Send onMessage;
        Closed onClosed;
    };

    struct BackendAdapterOptions {
        EventJournalConfig journal;
        UpdateBatchConfig batches;
        EventCoalescerConfig coalescer;
        std::size_t maxOutboundMessagesPerConnection = DefaultAdapterMaxOutboundMessages;
        std::size_t maxOutboundBytesPerConnection = DefaultAdapterMaxOutboundBytes;
        std::size_t maxMessagesPerDelivery = DefaultAdapterMaxMessagesPerDelivery;
        std::function<void(std::function<void()>)> scheduler;
    };

    enum class ConnectionReceiveStatus { Accepted, Rejected, Closing, Closed };

    struct ConnectionReceiveResult {
        ConnectionReceiveStatus status = ConnectionReceiveStatus::Rejected;
        std::optional<CodecError> error;

        [[nodiscard]] bool accepted() const noexcept {
            return status == ConnectionReceiveStatus::Accepted;
        }
    };

    class BackendAdapter;

    class FrontendConnection {
    public:
        FrontendConnection() noexcept;
        FrontendConnection(const FrontendConnection&) = delete;
        FrontendConnection(FrontendConnection&& other) noexcept;

        FrontendConnection& operator=(const FrontendConnection&) = delete;
        FrontendConnection& operator=(FrontendConnection&& other) noexcept;

        ~FrontendConnection();

        [[nodiscard]] ConnectionReceiveResult receive(const ClientMessage& message) noexcept;
        [[nodiscard]] ConnectionReceiveResult receive(const Json& message) noexcept;
        [[nodiscard]] ConnectionReceiveResult receive(std::string_view compactJson) noexcept;
        [[nodiscard]] ConnectionReceiveResult receiveError(CodecError error) noexcept;

        void close(std::string reason = "frontend connection closed") noexcept;

        [[nodiscard]] bool isOpen() const noexcept;
        [[nodiscard]] bool helloComplete() const noexcept;
        [[nodiscard]] std::optional<std::string> sessionId() const;
        [[nodiscard]] std::size_t queuedMessages() const noexcept;
        [[nodiscard]] std::size_t queuedBytes() const noexcept;

    private:
        friend class BackendAdapter;
        struct Control;

        explicit FrontendConnection(std::shared_ptr<Control> control) noexcept;

        std::shared_ptr<Control> control;
    };

    class BackendAdapter {
    public:
        template <typename ClientT>
        explicit BackendAdapter(backend::BackendCore<ClientT>& backend, BackendAdapterOptions options = {})
            : BackendAdapter(backend.implementation(), std::move(options)) {
        }

        BackendAdapter(const BackendAdapter&) = delete;
        BackendAdapter(BackendAdapter&&) = delete;

        BackendAdapter& operator=(const BackendAdapter&) = delete;
        BackendAdapter& operator=(BackendAdapter&&) = delete;

        ~BackendAdapter();

        [[nodiscard]] FrontendConnection openConnection(FrontendConnectionCallbacks callbacks);
        void flush();
        void close(std::string reason = "frontend adapter closed") noexcept;

        [[nodiscard]] bool isOpen() const noexcept;
        [[nodiscard]] bool flushScheduled() const noexcept;
        [[nodiscard]] SequenceNumber currentSequence() const noexcept;
        [[nodiscard]] std::size_t connectionCount() const noexcept;
        [[nodiscard]] EventJournalConfig journalConfig() const noexcept;
        [[nodiscard]] UpdateBatchConfig batchConfig() const noexcept;

    private:
        friend class FrontendConnection;

        explicit BackendAdapter(backend::detail::BackendCoreRuntime& backend, BackendAdapterOptions options);

        class Impl;
        std::shared_ptr<Impl> impl;
    };

} // namespace ai::openai::codex::frontend

#endif // AI_OPENAI_CODEX_FRONTEND_BACKENDADAPTER_H
