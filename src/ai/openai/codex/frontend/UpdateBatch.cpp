/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/frontend/UpdateBatch.h"

#include "ai/openai/codex/frontend/Codec.h"

#include <algorithm>
#include <compare>
#include <string>
#include <utility>

namespace ai::openai::codex::frontend {

    namespace {

        EventBatch makeBatch(std::vector<FrontendEvent> events) {
            EventBatch batch;
            batch.fromSequence = events.front().sequence;
            batch.toSequence = events.back().sequence;
            batch.events = std::move(events);
            return batch;
        }

        std::optional<std::size_t> encodedSize(const EventBatch& batch) {
            const auto serialized = Codec::serializeServer(ServerMessage{batch});
            if (!serialized) {
                return std::nullopt;
            }
            return serialized.value().size();
        }

    } // namespace

    UpdateBatchBuilder::UpdateBatchBuilder(UpdateBatchConfig config) noexcept
        : batchConfig(config) {
    }

    UpdateBatchConfig UpdateBatchBuilder::config() const noexcept {
        return batchConfig;
    }

    UpdateBatchResult UpdateBatchBuilder::build(const std::vector<FrontendEvent>& events) const noexcept {
        try {
            UpdateBatchResult result;
            if (batchConfig.maxEvents == 0 || batchConfig.maxSerializedBytes == 0) {
                result.status = UpdateBatchStatus::InvalidBounds;
                return result;
            }
            if (events.empty()) {
                result.status = UpdateBatchStatus::Success;
                return result;
            }
            for (std::size_t index = 1; index < events.size(); ++index) {
                if (events[index].sequence <= events[index - 1].sequence) {
                    result.status = UpdateBatchStatus::InvalidSequence;
                    return result;
                }
            }

            std::vector<FrontendEvent> pending;
            std::size_t pendingBytes = 0;
            pending.reserve(std::min(batchConfig.maxEvents, events.size()));

            for (const FrontendEvent& event : events) {
                std::vector<FrontendEvent> candidate = pending;
                candidate.push_back(event);
                const EventBatch candidateBatch = makeBatch(std::move(candidate));
                const auto candidateBytes = encodedSize(candidateBatch);
                if (!candidateBytes.has_value()) {
                    result.status = UpdateBatchStatus::EncodingFailure;
                    result.batches.clear();
                    return result;
                }

                const bool countExceeded = pending.size() + 1 > batchConfig.maxEvents;
                const bool bytesExceeded = *candidateBytes > batchConfig.maxSerializedBytes;
                if (!countExceeded && !bytesExceeded) {
                    pending = candidateBatch.events;
                    pendingBytes = *candidateBytes;
                    continue;
                }

                if (!pending.empty()) {
                    result.batches.push_back(BoundedEventBatch{makeBatch(std::move(pending)), pendingBytes});
                    pending.clear();
                    pending.reserve(std::min(batchConfig.maxEvents, events.size()));
                }

                std::vector<FrontendEvent> singleEvents{event};
                EventBatch single = makeBatch(std::move(singleEvents));
                const auto singleBytes = encodedSize(single);
                if (!singleBytes.has_value()) {
                    result.status = UpdateBatchStatus::EncodingFailure;
                    result.batches.clear();
                    return result;
                }
                if (*singleBytes > batchConfig.maxSerializedBytes) {
                    result.status = UpdateBatchStatus::SnapshotRequired;
                    result.oversizedSequence = event.sequence;
                    result.batches.clear();
                    return result;
                }
                pending = std::move(single.events);
                pendingBytes = *singleBytes;
            }

            if (!pending.empty()) {
                result.batches.push_back(BoundedEventBatch{makeBatch(std::move(pending)), pendingBytes});
            }
            result.status = UpdateBatchStatus::Success;
            return result;
        } catch (...) {
            return UpdateBatchResult{UpdateBatchStatus::EncodingFailure, {}, std::nullopt};
        }
    }

} // namespace ai::openai::codex::frontend
