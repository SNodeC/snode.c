/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_FRONTEND_EVENTJOURNAL_H
#define AI_OPENAI_CODEX_FRONTEND_EVENTJOURNAL_H

#include "ai/openai/codex/frontend/Messages.h"
#include "ai/openai/codex/frontend/Protocol.h"

#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace ai::openai::codex::frontend {

    struct EventJournalConfig {
        std::size_t maxEntries = DefaultJournalMaxEntries;
        std::size_t maxBytes = DefaultJournalMaxBytes;
        SequenceNumber initialSequence;

        bool operator==(const EventJournalConfig&) const = default;
    };

    enum class JournalAppendStatus { Appended, NotRetained, InvalidEvent, SequenceOverflow, EncodingFailure };

    struct JournalAppendResult {
        JournalAppendStatus status = JournalAppendStatus::EncodingFailure;
        std::optional<FrontendEvent> event;
        std::size_t serializedBytes = 0;

        [[nodiscard]] bool accepted() const noexcept {
            return status == JournalAppendStatus::Appended || status == JournalAppendStatus::NotRetained;
        }

        [[nodiscard]] bool retained() const noexcept {
            return status == JournalAppendStatus::Appended;
        }
    };

    enum class JournalReplayStatus { Available, Gap, FutureSequence };

    struct JournalReplayResult {
        JournalReplayStatus status = JournalReplayStatus::Gap;
        SequenceNumber requestedAfter;
        SequenceNumber oldestReplayableAfter;
        SequenceNumber currentSequence;
        std::vector<FrontendEvent> events;

        [[nodiscard]] bool requiresSnapshot() const noexcept {
            return status == JournalReplayStatus::Gap;
        }
    };

    class EventJournal {
    public:
        explicit EventJournal(EventJournalConfig config = {});

        [[nodiscard]] JournalAppendResult append(std::string type, Json data = Json::object(), Json extensions = Json::object()) noexcept;

        [[nodiscard]] JournalReplayResult replayAfter(SequenceNumber sequence) const;

        // Drops retained entries and advances a synchronization barrier without
        // fabricating a frontend event. Clients from before the barrier must
        // synchronize with a snapshot. Returns false when the sequence space is
        // exhausted; replay at the maximum sequence then remains unavailable.
        [[nodiscard]] bool invalidateReplay() noexcept;

        [[nodiscard]] EventJournalConfig config() const noexcept;
        [[nodiscard]] SequenceNumber currentSequence() const noexcept;
        [[nodiscard]] SequenceNumber oldestReplayableAfter() const noexcept;
        [[nodiscard]] std::optional<SequenceNumber> oldestRetainedSequence() const noexcept;
        [[nodiscard]] std::optional<SequenceNumber> newestRetainedSequence() const noexcept;
        [[nodiscard]] std::size_t retainedEntryCount() const noexcept;
        [[nodiscard]] std::size_t retainedBytes() const noexcept;
        [[nodiscard]] std::vector<FrontendEvent> retainedEvents() const;

    private:
        struct Entry {
            FrontendEvent event;
            std::size_t serializedBytes = 0;
        };

        void evictFront() noexcept;

        EventJournalConfig journalConfig;
        SequenceNumber current;
        SequenceNumber replayFloor;
        std::deque<Entry> entries;
        std::size_t byteCount = 0;
        bool replayAtCurrentUnavailable = false;
    };

} // namespace ai::openai::codex::frontend

#endif // AI_OPENAI_CODEX_FRONTEND_EVENTJOURNAL_H
