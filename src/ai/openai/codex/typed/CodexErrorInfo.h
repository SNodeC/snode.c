/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_CODEXERRORINFO_H
#define AI_OPENAI_CODEX_TYPED_CODEXERRORINFO_H

#include "ai/openai/codex/typed/Types.h"

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace ai::openai::codex::typed {

    template <typename T>
    struct OptionalNullable {
        bool present = false;
        std::optional<T> value;

        auto operator<=>(const OptionalNullable&) const = default;
    };

    struct NonSteerableTurnKind {
        std::string value;

        static NonSteerableTurnKind review();
        static NonSteerableTurnKind compact();

        auto operator<=>(const NonSteerableTurnKind&) const = default;
    };

#define CODEX_SCALAR_ERROR_INFO_TYPE(name) \
    struct name {                           \
        Json raw;                           \
    }

    CODEX_SCALAR_ERROR_INFO_TYPE(ContextWindowExceededCodexErrorInfo);
    CODEX_SCALAR_ERROR_INFO_TYPE(SessionBudgetExceededCodexErrorInfo);
    CODEX_SCALAR_ERROR_INFO_TYPE(UsageLimitExceededCodexErrorInfo);
    CODEX_SCALAR_ERROR_INFO_TYPE(ServerOverloadedCodexErrorInfo);
    CODEX_SCALAR_ERROR_INFO_TYPE(CyberPolicyCodexErrorInfo);
    CODEX_SCALAR_ERROR_INFO_TYPE(InternalServerErrorCodexErrorInfo);
    CODEX_SCALAR_ERROR_INFO_TYPE(UnauthorizedCodexErrorInfo);
    CODEX_SCALAR_ERROR_INFO_TYPE(BadRequestCodexErrorInfo);
    CODEX_SCALAR_ERROR_INFO_TYPE(ThreadRollbackFailedCodexErrorInfo);
    CODEX_SCALAR_ERROR_INFO_TYPE(SandboxErrorCodexErrorInfo);
    CODEX_SCALAR_ERROR_INFO_TYPE(OtherCodexErrorInfo);

#undef CODEX_SCALAR_ERROR_INFO_TYPE

#define CODEX_HTTP_ERROR_INFO_TYPE(name)                    \
    struct name {                                           \
        OptionalNullable<std::uint16_t> httpStatusCode;      \
        Json raw;                                            \
    }

    CODEX_HTTP_ERROR_INFO_TYPE(HttpConnectionFailedCodexErrorInfo);
    CODEX_HTTP_ERROR_INFO_TYPE(ResponseStreamConnectionFailedCodexErrorInfo);
    CODEX_HTTP_ERROR_INFO_TYPE(ResponseStreamDisconnectedCodexErrorInfo);
    CODEX_HTTP_ERROR_INFO_TYPE(ResponseTooManyFailedAttemptsCodexErrorInfo);

#undef CODEX_HTTP_ERROR_INFO_TYPE

    struct ActiveTurnNotSteerableCodexErrorInfo {
        NonSteerableTurnKind turnKind;
        Json raw;
    };

    struct UnknownCodexErrorInfo {
        std::optional<std::string> discriminator;
        Json raw;
        DecodeDiagnostic diagnostic;
    };

    using CodexErrorInfo = std::variant<ContextWindowExceededCodexErrorInfo,
                                        SessionBudgetExceededCodexErrorInfo,
                                        UsageLimitExceededCodexErrorInfo,
                                        ServerOverloadedCodexErrorInfo,
                                        CyberPolicyCodexErrorInfo,
                                        InternalServerErrorCodexErrorInfo,
                                        UnauthorizedCodexErrorInfo,
                                        BadRequestCodexErrorInfo,
                                        ThreadRollbackFailedCodexErrorInfo,
                                        SandboxErrorCodexErrorInfo,
                                        OtherCodexErrorInfo,
                                        HttpConnectionFailedCodexErrorInfo,
                                        ResponseStreamConnectionFailedCodexErrorInfo,
                                        ResponseStreamDisconnectedCodexErrorInfo,
                                        ResponseTooManyFailedAttemptsCodexErrorInfo,
                                        ActiveTurnNotSteerableCodexErrorInfo,
                                        UnknownCodexErrorInfo>;

    struct TurnError {
        std::string message;
        OptionalNullable<std::string> additionalDetails;
        OptionalNullable<CodexErrorInfo> codexErrorInfo;
        std::optional<DecodeDiagnostic> codexErrorDiagnostic;
        Json raw;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_CODEXERRORINFO_H
