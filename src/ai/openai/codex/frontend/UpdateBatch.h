/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_FRONTEND_UPDATEBATCH_H
#define AI_OPENAI_CODEX_FRONTEND_UPDATEBATCH_H

#include "ai/openai/codex/frontend/Messages.h"
#include "ai/openai/codex/frontend/Protocol.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace ai::openai::codex::frontend {

    struct UpdateBatchConfig {
        std::size_t maxEvents = DefaultBatchMaxEvents;
        std::size_t maxSerializedBytes = DefaultBatchMaxBytes;

        bool operator==(const UpdateBatchConfig&) const = default;
    };

    struct BoundedEventBatch {
        EventBatch batch;
        std::size_t serializedBytes = 0;

        bool operator==(const BoundedEventBatch&) const = default;
    };

    enum class UpdateBatchStatus { Success, SnapshotRequired, InvalidSequence, InvalidBounds, EncodingFailure };

    struct UpdateBatchResult {
        UpdateBatchStatus status = UpdateBatchStatus::EncodingFailure;
        std::vector<BoundedEventBatch> batches;
        std::optional<SequenceNumber> oversizedSequence;

        [[nodiscard]] bool success() const noexcept {
            return status == UpdateBatchStatus::Success;
        }

        [[nodiscard]] bool requiresSnapshot() const noexcept {
            return status == UpdateBatchStatus::SnapshotRequired;
        }
    };

    class UpdateBatchBuilder {
    public:
        explicit UpdateBatchBuilder(UpdateBatchConfig config = {}) noexcept;

        [[nodiscard]] UpdateBatchConfig config() const noexcept;
        [[nodiscard]] UpdateBatchResult build(const std::vector<FrontendEvent>& events) const noexcept;

    private:
        UpdateBatchConfig batchConfig;
    };

} // namespace ai::openai::codex::frontend

#endif // AI_OPENAI_CODEX_FRONTEND_UPDATEBATCH_H
