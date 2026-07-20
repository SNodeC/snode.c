/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_FRONTEND_EVENTCOALESCER_H
#define AI_OPENAI_CODEX_FRONTEND_EVENTCOALESCER_H

#include "ai/openai/codex/frontend/Messages.h"
#include "ai/openai/codex/frontend/Protocol.h"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ai::openai::codex::frontend {

    enum class DirtyEntityKind {
        BackendLifecycle,
        Diagnostic,
        Thread,
        Turn,
        Item,
        ItemContent,
        PendingRequest,
        Controller,
        Session,
        CodexExtension
    };

    struct CoalescingKey {
        DirtyEntityKind kind = DirtyEntityKind::BackendLifecycle;
        std::string threadId;
        std::string turnId;
        std::string entityId;
        std::string channel;

        [[nodiscard]] static CoalescingKey thread(std::string threadId);
        [[nodiscard]] static CoalescingKey turn(std::string threadId, std::string turnId);
        [[nodiscard]] static CoalescingKey item(std::string threadId, std::string turnId, std::string itemId);
        [[nodiscard]] static CoalescingKey itemContent(std::string threadId, std::string turnId, std::string itemId, std::string channel);
        [[nodiscard]] static CoalescingKey pendingRequest(std::string requestId);

        auto operator<=>(const CoalescingKey&) const = default;
    };

    enum class FlushUrgency { Deferred, Immediate };

    struct DirtyUpdate {
        CoalescingKey key;
        std::string type;
        Json data = Json::object();
        FlushUrgency urgency = FlushUrgency::Deferred;

        bool operator==(const DirtyUpdate&) const = default;
    };

    struct EventCoalescerConfig {
        std::size_t maxDirtyEntities = DefaultMaxDirtyEntities;

        bool operator==(const EventCoalescerConfig&) const = default;
    };

    enum class CoalescerMarkStatus { Accepted, SnapshotRequired, InvalidUpdate, AllocationFailure };

    struct CoalescerMarkResult {
        CoalescerMarkStatus status = CoalescerMarkStatus::InvalidUpdate;
        bool scheduleRequired = false;
        bool immediateFlush = false;

        [[nodiscard]] bool accepted() const noexcept {
            return status == CoalescerMarkStatus::Accepted;
        }
    };

    struct CoalescerDrainResult {
        std::vector<DirtyUpdate> updates;
        bool snapshotRequired = false;
    };

    class EventCoalescer {
    public:
        explicit EventCoalescer(EventCoalescerConfig config = {}) noexcept;

        [[nodiscard]] CoalescerMarkResult mark(DirtyUpdate update) noexcept;
        [[nodiscard]] CoalescerMarkResult requireSnapshot(FlushUrgency urgency = FlushUrgency::Immediate) noexcept;
        [[nodiscard]] CoalescerDrainResult drain();
        void clear() noexcept;

        [[nodiscard]] EventCoalescerConfig config() const noexcept;
        [[nodiscard]] bool flushScheduled() const noexcept;
        [[nodiscard]] bool snapshotRequired() const noexcept;
        [[nodiscard]] std::size_t dirtyCount() const noexcept;

    private:
        struct StoredUpdate {
            DirtyUpdate update;
            std::uint64_t insertionOrder = 0;
        };

        EventCoalescerConfig coalescerConfig;
        std::map<CoalescingKey, StoredUpdate> dirty;
        std::uint64_t nextInsertionOrder = 0;
        bool scheduled = false;
        bool needsSnapshot = false;
    };

} // namespace ai::openai::codex::frontend

#endif // AI_OPENAI_CODEX_FRONTEND_EVENTCOALESCER_H
