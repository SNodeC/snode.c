/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_TURNCODEC_H
#define AI_OPENAI_CODEX_DETAIL_TURNCODEC_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/typed/Conversation.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/Turns.h"
#include "ai/openai/codex/typed/Types.h"

#include <optional>
#include <string>
#include <vector>

namespace ai::openai::codex::detail {

    // Event paths may preserve malformed known item payloads as UnknownItem;
    // successful operation results use this helper to reject that condition
    // locally without failing the underlying protocol connection.
    bool hasMalformedKnownPayload(const typed::Turn& turn) noexcept;

    std::optional<Json> encodeTurnInterruptParams(const typed::TurnInterruptParams& params, std::string& error);
    std::optional<Json> encodeTurnStartParams(const typed::TurnStartParams& params, std::string& error);
    std::optional<Json> encodeTurnSteerParams(const typed::TurnSteerParams& params, std::string& error);

    std::optional<typed::Turn> decodeTurn(const Json& value, const typed::ThreadId& threadId, std::string& error);
    std::optional<typed::TurnStartResponse>
    decodeTurnStartResponse(const Json& value, const typed::ThreadId& threadId, std::string& error);
    std::optional<typed::TurnSteerResponse> decodeTurnSteerResponse(const Json& value, std::string& error);
    std::optional<typed::Unit> decodeTurnInterruptResult(const Json& value, std::string& error);

    // A1.0 compatibility codec entrypoints.
    std::optional<Json> encodeTurnInput(const typed::TurnInput& input, std::string& error);
    std::optional<Json> encodeTurnStartParams(const typed::ThreadId& threadId,
                                              const std::vector<typed::TurnInput>& input,
                                              const typed::TurnStartOptions& options,
                                              std::string& error);
    std::optional<Json>
    encodeTurnInterruptParams(const typed::ThreadId& threadId, const typed::TurnId& turnId, std::string& error);
    std::optional<typed::Turn> decodeTurnStartResult(const Json& value, const typed::ThreadId& threadId, std::string& error);

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_TURNCODEC_H
