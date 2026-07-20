/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_RESULTS_H
#define AI_OPENAI_CODEX_TYPED_RESULTS_H

#include "ai/openai/codex/Protocol.h"

#include <optional>

namespace ai::openai::codex::typed {

    template <typename T>
    struct OperationResult {
        enum class Kind { Success, RemoteError, Cancelled, LocalError };

        Kind kind = Kind::Success;
        std::optional<T> value;
        std::optional<ProtocolError> remoteError;
        std::optional<Error> localError;
        std::optional<ClientRequestId> requestId;
        Json raw = nullptr;

        explicit operator bool() const noexcept {
            return kind == Kind::Success && value.has_value();
        }
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_RESULTS_H
