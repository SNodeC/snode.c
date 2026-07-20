/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/frontend/EventCoalescer.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace ai::openai::codex::frontend {

    CoalescingKey CoalescingKey::thread(std::string threadId) {
        return {DirtyEntityKind::Thread, std::move(threadId), {}, {}, {}};
    }

    CoalescingKey CoalescingKey::turn(std::string threadId, std::string turnId) {
        return {DirtyEntityKind::Turn, std::move(threadId), std::move(turnId), {}, {}};
    }

    CoalescingKey CoalescingKey::item(std::string threadId, std::string turnId, std::string itemId) {
        return {DirtyEntityKind::Item, std::move(threadId), std::move(turnId), std::move(itemId), {}};
    }

    CoalescingKey CoalescingKey::itemContent(std::string threadId, std::string turnId, std::string itemId, std::string channel) {
        return {DirtyEntityKind::ItemContent, std::move(threadId), std::move(turnId), std::move(itemId), std::move(channel)};
    }

    CoalescingKey CoalescingKey::pendingRequest(std::string requestId) {
        return {DirtyEntityKind::PendingRequest, {}, {}, std::move(requestId), {}};
    }

    EventCoalescer::EventCoalescer(EventCoalescerConfig config) noexcept
        : coalescerConfig(config) {
    }

    CoalescerMarkResult EventCoalescer::mark(DirtyUpdate update) noexcept {
        const bool scheduleRequired = !scheduled;
        const bool immediate = update.urgency == FlushUrgency::Immediate;
        if (update.type.empty() || !update.data.is_object()) {
            return {CoalescerMarkStatus::InvalidUpdate, false, immediate};
        }

        try {
            const auto existing = dirty.find(update.key);
            if (existing != dirty.end()) {
                existing->second.update.type = std::move(update.type);
                existing->second.update.data = std::move(update.data);
                if (immediate) {
                    if (nextInsertionOrder == std::numeric_limits<std::uint64_t>::max()) {
                        needsSnapshot = true;
                        scheduled = true;
                        return {CoalescerMarkStatus::SnapshotRequired, scheduleRequired, true};
                    }
                    existing->second.update.urgency = FlushUrgency::Immediate;
                    // A terminal/interactive replacement belongs after all
                    // dirty state observed before it. In particular, final
                    // item content must precede item/turn completion updates
                    // even when the same turn was first marked dirty earlier
                    // in this tick.
                    existing->second.insertionOrder = nextInsertionOrder++;
                }
                scheduled = true;
                return {CoalescerMarkStatus::Accepted, scheduleRequired, immediate};
            }

            if (coalescerConfig.maxDirtyEntities == 0 || dirty.size() >= coalescerConfig.maxDirtyEntities ||
                nextInsertionOrder == std::numeric_limits<std::uint64_t>::max()) {
                needsSnapshot = true;
                scheduled = true;
                return {CoalescerMarkStatus::SnapshotRequired, scheduleRequired, true};
            }

            const CoalescingKey key = update.key;
            dirty.emplace(key, StoredUpdate{std::move(update), nextInsertionOrder++});
            scheduled = true;
            return {CoalescerMarkStatus::Accepted, scheduleRequired, immediate};
        } catch (...) {
            needsSnapshot = true;
            scheduled = true;
            return {CoalescerMarkStatus::AllocationFailure, scheduleRequired, true};
        }
    }

    CoalescerMarkResult EventCoalescer::requireSnapshot(FlushUrgency urgency) noexcept {
        const bool scheduleRequired = !scheduled;
        needsSnapshot = true;
        scheduled = true;
        return {CoalescerMarkStatus::SnapshotRequired, scheduleRequired, urgency == FlushUrgency::Immediate};
    }

    CoalescerDrainResult EventCoalescer::drain() {
        CoalescerDrainResult result;
        result.snapshotRequired = needsSnapshot;
        result.updates.reserve(dirty.size());

        std::vector<const StoredUpdate*> ordered;
        ordered.reserve(dirty.size());
        for (const auto& [key, stored] : dirty) {
            (void) key;
            ordered.push_back(&stored);
        }
        std::sort(ordered.begin(), ordered.end(), [](const StoredUpdate* left, const StoredUpdate* right) {
            return left->insertionOrder < right->insertionOrder;
        });
        for (const StoredUpdate* stored : ordered) {
            result.updates.push_back(stored->update);
        }

        clear();
        return result;
    }

    void EventCoalescer::clear() noexcept {
        dirty.clear();
        nextInsertionOrder = 0;
        scheduled = false;
        needsSnapshot = false;
    }

    EventCoalescerConfig EventCoalescer::config() const noexcept {
        return coalescerConfig;
    }

    bool EventCoalescer::flushScheduled() const noexcept {
        return scheduled;
    }

    bool EventCoalescer::snapshotRequired() const noexcept {
        return needsSnapshot;
    }

    std::size_t EventCoalescer::dirtyCount() const noexcept {
        return dirty.size();
    }

} // namespace ai::openai::codex::frontend
