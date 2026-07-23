/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_CODEXERRORINFOCODEC_H
#define AI_OPENAI_CODEX_DETAIL_CODEXERRORINFOCODEC_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/typed/CodexErrorInfo.h"

#include <optional>
#include <span>

namespace ai::openai::codex::detail {

    struct CodexErrorInfoDecodeResult {
        std::optional<typed::CodexErrorInfo> value;
        std::optional<typed::DecodeDiagnostic> diagnostic;
    };

    CodexErrorInfoDecodeResult decodeCodexErrorInfo(const Json& value) noexcept;

    std::span<const CodexErrorInfoCodecDescriptor> codexErrorInfoCodecDescriptors() noexcept;

    std::optional<typed::TurnError>
    decodeTurnError(const Json& value, std::optional<typed::DecodeDiagnostic>& diagnostic) noexcept;

    void decodeProtocolErrorTurnInfo(const ProtocolError& error,
                                     std::optional<typed::CodexErrorInfo>& codexErrorInfo,
                                     std::optional<typed::DecodeDiagnostic>& diagnostic) noexcept;

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_CODEXERRORINFOCODEC_H
