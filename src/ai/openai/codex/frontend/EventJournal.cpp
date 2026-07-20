/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/frontend/EventJournal.h"

#include "ai/openai/codex/frontend/Codec.h"

#include <exception>
#include <limits>
#include <utility>

namespace ai::openai::codex::frontend {

    EventJournal::EventJournal(EventJournalConfig config)
        : journalConfig(config)
        , current(config.initialSequence)
        , replayFloor(config.initialSequence) {
    }

    JournalAppendResult EventJournal::append(std::string type, Json data, Json extensions) noexcept {
        try {
            if (type.empty() || !data.is_object() || !extensions.is_object()) {
                return {JournalAppendStatus::InvalidEvent, std::nullopt, 0};
            }
            if (current == SequenceNumber::maximum()) {
                return {JournalAppendStatus::SequenceOverflow, std::nullopt, 0};
            }

            const SequenceNumber next(current.value() + 1);
            FrontendEvent event{next, std::move(type), std::move(data), std::move(extensions)};
            const auto encodedSize = Codec::serializedEventSize(event);
            if (!encodedSize) {
                return {JournalAppendStatus::EncodingFailure, std::nullopt, 0};
            }

            JournalAppendResult result{JournalAppendStatus::Appended, event, encodedSize.value()};
            if (journalConfig.maxEntries == 0 || journalConfig.maxBytes == 0 || encodedSize.value() > journalConfig.maxBytes) {
                entries.clear();
                byteCount = 0;
                current = next;
                replayFloor = next;
                result.status = JournalAppendStatus::NotRetained;
                return result;
            }

            // Push before evicting so allocation failure cannot leave a partly
            // mutated journal. encodedSize <= maxBytes guarantees the final
            // byte-count addition cannot overflow after eviction.
            entries.push_back(Entry{event, encodedSize.value()});
            current = next;
            while (entries.size() > 1 &&
                   (entries.size() > journalConfig.maxEntries || byteCount > journalConfig.maxBytes - encodedSize.value())) {
                evictFront();
            }
            byteCount += encodedSize.value();
            return result;
        } catch (...) {
            return {JournalAppendStatus::EncodingFailure, std::nullopt, 0};
        }
    }

    JournalReplayResult EventJournal::replayAfter(SequenceNumber sequence) const {
        JournalReplayResult result;
        result.requestedAfter = sequence;
        result.oldestReplayableAfter = replayFloor;
        result.currentSequence = current;

        if (sequence > current) {
            result.status = JournalReplayStatus::FutureSequence;
            return result;
        }
        if (sequence < replayFloor || (replayAtCurrentUnavailable && sequence == current)) {
            result.status = JournalReplayStatus::Gap;
            return result;
        }

        result.status = JournalReplayStatus::Available;
        for (const Entry& entry : entries) {
            if (entry.event.sequence > sequence) {
                result.events.push_back(entry.event);
            }
        }
        return result;
    }

    bool EventJournal::invalidateReplay() noexcept {
        entries.clear();
        byteCount = 0;
        if (current == SequenceNumber::maximum()) {
            replayFloor = current;
            replayAtCurrentUnavailable = true;
            return false;
        }
        current = SequenceNumber(current.value() + 1);
        replayFloor = current;
        replayAtCurrentUnavailable = false;
        return true;
    }

    EventJournalConfig EventJournal::config() const noexcept {
        return journalConfig;
    }

    SequenceNumber EventJournal::currentSequence() const noexcept {
        return current;
    }

    SequenceNumber EventJournal::oldestReplayableAfter() const noexcept {
        return replayFloor;
    }

    std::optional<SequenceNumber> EventJournal::oldestRetainedSequence() const noexcept {
        if (entries.empty()) {
            return std::nullopt;
        }
        return entries.front().event.sequence;
    }

    std::optional<SequenceNumber> EventJournal::newestRetainedSequence() const noexcept {
        if (entries.empty()) {
            return std::nullopt;
        }
        return entries.back().event.sequence;
    }

    std::size_t EventJournal::retainedEntryCount() const noexcept {
        return entries.size();
    }

    std::size_t EventJournal::retainedBytes() const noexcept {
        return byteCount;
    }

    std::vector<FrontendEvent> EventJournal::retainedEvents() const {
        std::vector<FrontendEvent> result;
        result.reserve(entries.size());
        for (const Entry& entry : entries) {
            result.push_back(entry.event);
        }
        return result;
    }

    void EventJournal::evictFront() noexcept {
        if (entries.empty()) {
            return;
        }
        replayFloor = entries.front().event.sequence;
        byteCount -= entries.front().serializedBytes;
        entries.pop_front();
    }

} // namespace ai::openai::codex::frontend
