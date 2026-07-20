/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_BACKEND_SNAPSHOT_H
#define AI_OPENAI_CODEX_BACKEND_SNAPSHOT_H

#include "ai/openai/codex/backend/BackendState.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace ai::openai::codex::backend {

    // Public snapshots deliberately use stricter, transport-friendly bounds
    // than the canonical reducer state. A normalized codex.extension event
    // built from one ExtensionSnapshot therefore fits the default frontend
    // batch bound even for adversarial string escaping.
    inline constexpr std::size_t MaxSnapshotCodexExtensions = 64;
    inline constexpr std::size_t MaxSnapshotExtensionMethodBytes = 1024;
    inline constexpr std::size_t MaxSnapshotExtensionPayloadBytes = 32U * 1024U;
    inline constexpr std::size_t MaxSnapshotExtensionDecodingErrorBytes = 2U * 1024U;
    inline constexpr std::size_t MaxSerializedCodexExtensionEventBytes = 64U * 1024U;
    inline constexpr std::size_t MaxSerializedCodexExtensionEnvelopeOverheadBytes = 4U * 1024U;
    static_assert(MaxSnapshotExtensionMethodBytes * 6U + MaxSnapshotExtensionPayloadBytes + MaxSnapshotExtensionDecodingErrorBytes * 6U +
                          MaxSerializedCodexExtensionEnvelopeOverheadBytes <=
                      MaxSerializedCodexExtensionEventBytes,
                  "escaped extension fields plus the normalized envelope must remain within the documented event bound");

    struct ErrorSnapshot {
        std::string category;
        std::int64_t code = 0;
        std::string message;

        bool operator==(const ErrorSnapshot&) const = default;
    };

    struct ItemSnapshot {
        std::string id;
        std::string type;
        std::string status;
        std::string agentText;
        std::string reasoningText;
        std::string reasoningSummary;
        std::string commandOutput;
        std::uint64_t droppedContentBytes = 0;
        bool contentTruncated = false;
        std::optional<std::int64_t> startedAtMs;
        std::optional<std::int64_t> completedAtMs;
        Json data = Json::object();
        Json extensions = Json::object();

        bool operator==(const ItemSnapshot&) const = default;
    };

    struct TurnSnapshot {
        std::string id;
        std::string threadId;
        std::string status;
        bool active = false;
        bool terminal = false;
        std::optional<Json> failure;
        std::optional<Json> tokenUsage;
        std::vector<ItemSnapshot> items;
        Json extensions = Json::object();

        bool operator==(const TurnSnapshot&) const = default;
    };

    struct ThreadSnapshot {
        std::string id;
        std::optional<std::string> title;
        std::optional<std::string> cwd;
        std::optional<std::string> model;
        std::optional<std::string> modelProvider;
        std::optional<std::string> preview;
        std::optional<std::string> status;
        std::optional<std::int64_t> createdAt;
        std::optional<std::int64_t> updatedAt;
        bool fullyLoaded = false;
        std::vector<TurnSnapshot> turns;
        Json extensions = Json::object();

        bool operator==(const ThreadSnapshot&) const = default;
    };

    struct PendingRequestSnapshot {
        PendingRequestId id;
        std::string type;
        std::optional<std::string> threadId;
        std::optional<std::string> turnId;
        std::optional<std::string> itemId;
        Json details = Json::object();

        bool operator==(const PendingRequestSnapshot&) const = default;
    };

    struct SessionSnapshot {
        SessionId id;
        SessionRole role = SessionRole::Observer;

        bool operator==(const SessionSnapshot&) const = default;
    };

    struct ThreadListSnapshot {
        bool hasLoadedPage = false;
        bool complete = false;
        std::optional<std::string> nextCursor;
        std::optional<std::string> backwardsCursor;
        std::size_t pagesLoaded = 0;

        bool operator==(const ThreadListSnapshot&) const = default;
    };

    struct ReplayRangeSnapshot {
        SequenceNumber oldest;
        SequenceNumber newest;

        bool operator==(const ReplayRangeSnapshot&) const = default;
    };

    struct ExtensionSnapshot {
        std::string method;
        Json payload = nullptr;
        std::optional<std::string> decodingError;
        bool methodTruncated = false;
        bool payloadTruncated = false;
        bool decodingErrorTruncated = false;
        bool sensitiveFieldsRedacted = false;
        std::uint64_t originalMethodBytes = 0;
        std::optional<std::uint64_t> originalPayloadBytes;
        std::uint64_t originalDecodingErrorBytes = 0;

        bool operator==(const ExtensionSnapshot&) const = default;
    };

    struct Snapshot {
        SequenceNumber sequence;
        BackendLifecycle lifecycle = BackendLifecycle::Stopped;
        std::optional<ErrorSnapshot> lastLifecycleError;
        DiagnosticSummary diagnostics;
        std::vector<ThreadSnapshot> threads;
        std::vector<PendingRequestSnapshot> pendingRequests;
        std::optional<SessionId> controller;
        std::vector<SessionSnapshot> sessions;
        ThreadListSnapshot threadList;
        std::optional<ReplayRangeSnapshot> replayRange;
        std::vector<ExtensionSnapshot> recentExtensions;
        std::size_t omittedRecentExtensions = 0;
        bool sequenceExhausted = false;

        bool operator==(const Snapshot&) const;
        bool operator!=(const Snapshot&) const;
    };

    Snapshot makeSnapshot(const BackendState& state);
    ExtensionSnapshot makeExtensionSnapshot(const ExtensionRecord& extension);

} // namespace ai::openai::codex::backend

#endif // AI_OPENAI_CODEX_BACKEND_SNAPSHOT_H
