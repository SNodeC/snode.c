/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_CLIENTOPERATIONCODEC_H
#define AI_OPENAI_CODEX_DETAIL_CLIENTOPERATIONCODEC_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/typed/Accounts.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/Threads.h"
#include "ai/openai/codex/typed/Turns.h"
#include "ai/openai/codex/typed/Types.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace ai::openai::codex::detail {

    enum class ClientOperationDecodeCode {
        Decoded,
        MalformedKnownPayload,
        MissingContext,
        RegistryDescriptorMismatch,
        ResultTypeMismatch,
        UnsupportedTarget
    };

    [[nodiscard]] std::string_view clientOperationDecodeCodeName(ClientOperationDecodeCode code) noexcept;
    [[nodiscard]] std::string_view clientOperationTargetIdentity(ClientRequestTarget target) noexcept;

    using ClientOperationDecodedValue = std::variant<typed::Unit,
                                                     typed::CancelLoginAccountResponse,
                                                     typed::LoginAccountResponse,
                                                     typed::ConsumeAccountRateLimitResetCreditResponse,
                                                     typed::GetAccountRateLimitsResponse,
                                                     typed::GetAccountResponse,
                                                     typed::SendAddCreditsNudgeEmailResponse,
                                                     typed::GetAccountTokenUsageResponse,
                                                     typed::GetWorkspaceMessagesResponse,
                                                     typed::ThreadForkResponse,
                                                     typed::ThreadGoalClearResponse,
                                                     typed::ThreadGoalGetResponse,
                                                     typed::ThreadGoalSetResponse,
                                                     typed::ThreadListResponse,
                                                     typed::ThreadLoadedListResponse,
                                                     typed::ThreadMetadataUpdateResponse,
                                                     typed::ThreadReadResponse,
                                                     typed::ThreadResumeResponse,
                                                     typed::ThreadRollbackResponse,
                                                     typed::ThreadStartResponse,
                                                     typed::ThreadUnarchiveResponse,
                                                     typed::ThreadUnsubscribeResponse,
                                                     typed::TurnStartResponse,
                                                     typed::TurnSteerResponse>;

    struct ClientOperationDecodeDiagnostic {
        ClientOperationDecodeCode code = ClientOperationDecodeCode::MalformedKnownPayload;
        ClientRequestTarget target = ClientRequestTarget::Count;
        ProtocolSurfaceKey surfaceKey;
        std::string fieldPath = "$";
        std::string message;
    };

    struct ClientOperationDecodeResult {
        std::optional<ClientOperationDecodedValue> value;
        ClientOperationDecodeDiagnostic diagnostic;

        [[nodiscard]] explicit operator bool() const noexcept {
            return value.has_value() && diagnostic.code == ClientOperationDecodeCode::Decoded;
        }
    };

    ClientOperationDecodeResult decodeClientOperationResult(ClientRequestTarget target,
                                                            const Json& raw,
                                                            std::optional<typed::ThreadId> contextualThreadId = std::nullopt) noexcept;

    template <typename Result>
    std::optional<Result> decodeClientOperationResultAs(ClientRequestTarget target,
                                                        const Json& raw,
                                                        std::optional<typed::ThreadId> contextualThreadId,
                                                        std::string& error) {
        ClientOperationDecodeResult decoded = decodeClientOperationResult(target, raw, std::move(contextualThreadId));
        if (!decoded) {
            error = decoded.diagnostic.message;
            return std::nullopt;
        }
        Result* value = std::get_if<Result>(&*decoded.value);
        if (value == nullptr) {
            error = "typed App Server result target has an incompatible C++ result type";
            return std::nullopt;
        }
        error.clear();
        return std::move(*value);
    }

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_CLIENTOPERATIONCODEC_H
