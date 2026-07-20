/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_BACKEND_BACKENDSTATE_H
#define AI_OPENAI_CODEX_BACKEND_BACKENDSTATE_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/typed/Client.h"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace ai::openai::codex::backend {

    class SessionId {
    public:
        constexpr SessionId() noexcept = default;
        explicit constexpr SessionId(std::uint64_t value) noexcept
            : id(value) {
        }

        constexpr std::uint64_t value() const noexcept {
            return id;
        }

        constexpr explicit operator bool() const noexcept {
            return id != 0;
        }

        auto operator<=>(const SessionId&) const = default;

    private:
        std::uint64_t id = 0;
    };

    class PendingRequestId {
    public:
        constexpr PendingRequestId() noexcept = default;
        explicit constexpr PendingRequestId(std::uint64_t value) noexcept
            : id(value) {
        }

        constexpr std::uint64_t value() const noexcept {
            return id;
        }

        constexpr explicit operator bool() const noexcept {
            return id != 0;
        }

        auto operator<=>(const PendingRequestId&) const = default;

    private:
        std::uint64_t id = 0;
    };

    class SequenceNumber {
    public:
        constexpr SequenceNumber() noexcept = default;
        explicit constexpr SequenceNumber(std::uint64_t value) noexcept
            : sequence(value) {
        }

        constexpr std::uint64_t value() const noexcept {
            return sequence;
        }

        auto operator<=>(const SequenceNumber&) const = default;

    private:
        std::uint64_t sequence = 0;
    };

    enum class BackendLifecycle { Stopped, Starting, Initializing, Ready, Stopping, Failed };
    enum class SessionRole { Observer, Controller };
    enum class ItemLifecycle { Unknown, Started, Completed, Failed };

    struct DiagnosticSummary {
        std::uint64_t received = 0;
        std::vector<std::string> recent;
    };

    struct ModelRerouteRecord {
        typed::ModelId from;
        typed::ModelId to;
        std::string reason;
    };

    struct ItemState {
        typed::Item item;
        ItemLifecycle lifecycle = ItemLifecycle::Unknown;
        std::string agentText;
        std::string reasoningText;
        std::string reasoningSummary;
        std::string commandOutput;
        std::uint64_t droppedContentBytes = 0;
        std::optional<std::int64_t> startedAtMs;
        std::optional<std::int64_t> completedAtMs;
        Json extensions = Json::object();
    };

    struct TurnState {
        typed::Turn turn;
        std::map<std::string, ItemState> items;
        std::vector<typed::ItemId> itemOrder;
        bool active = false;
        bool terminal = false;
        std::optional<Json> failure;
        std::optional<Json> tokenUsage;
        std::vector<ModelRerouteRecord> modelReroutes;
        Json extensions = Json::object();
    };

    struct ThreadState {
        typed::Thread thread;
        std::map<std::string, TurnState> turns;
        std::vector<typed::TurnId> turnOrder;
        bool fullyLoaded = false;
        Json extensions = Json::object();
    };

    struct PendingRequestState {
        PendingRequestId id;
        typed::TypedServerRequest request;
        std::uint64_t connectionGeneration = 0;
    };

    struct ConnectedSessionState {
        SessionId id;
        SessionRole role = SessionRole::Observer;
    };

    struct ThreadListState {
        bool hasLoadedPage = false;
        bool complete = false;
        std::optional<std::string> nextCursor;
        std::optional<std::string> backwardsCursor;
        std::size_t pagesLoaded = 0;
    };

    struct ExtensionRecord {
        std::string method;
        Json payload = nullptr;
        std::optional<std::string> decodingError;
        std::optional<std::uint64_t> originalMethodBytes;
        std::optional<std::uint64_t> originalPayloadBytes;
        std::optional<std::uint64_t> originalDecodingErrorBytes;
    };

    struct BackendState {
        BackendLifecycle lifecycle = BackendLifecycle::Stopped;
        std::optional<Error> lastLifecycleError;
        DiagnosticSummary diagnostics;
        std::map<std::string, ThreadState> threads;
        std::vector<typed::ThreadId> threadOrder;
        std::map<PendingRequestId, PendingRequestState> pendingRequests;
        std::map<SessionId, ConnectedSessionState> sessions;
        std::optional<SessionId> controller;
        SequenceNumber sequence;
        bool sequenceExhausted = false;
        ThreadListState threadList;
        std::vector<ExtensionRecord> recentExtensions;
    };

    BackendLifecycle toBackendLifecycle(State state) noexcept;
    bool isTerminal(const typed::TurnStatus& status) noexcept;
    std::optional<typed::ItemId> itemId(const typed::Item& item);
    std::string itemType(const typed::Item& item);

    ThreadState* findThread(BackendState& state, const typed::ThreadId& threadId) noexcept;
    const ThreadState* findThread(const BackendState& state, const typed::ThreadId& threadId) noexcept;
    TurnState* findTurn(BackendState& state, const typed::ThreadId& threadId, const typed::TurnId& turnId) noexcept;
    const TurnState* findTurn(const BackendState& state, const typed::ThreadId& threadId, const typed::TurnId& turnId) noexcept;
    ItemState*
    findItem(BackendState& state, const typed::ThreadId& threadId, const typed::TurnId& turnId, const typed::ItemId& itemId) noexcept;
    const ItemState*
    findItem(const BackendState& state, const typed::ThreadId& threadId, const typed::TurnId& turnId, const typed::ItemId& itemId) noexcept;

} // namespace ai::openai::codex::backend

#endif // AI_OPENAI_CODEX_BACKEND_BACKENDSTATE_H
