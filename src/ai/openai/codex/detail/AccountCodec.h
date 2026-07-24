/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_ACCOUNTCODEC_H
#define AI_OPENAI_CODEX_DETAIL_ACCOUNTCODEC_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ConversationCodec.h"
#include "ai/openai/codex/typed/Accounts.h"

#include <optional>
#include <string>

namespace ai::openai::codex::detail {

    ConversationDecodeResult<typed::Account> decodeAccount(const Json& value) noexcept;
    ConversationDecodeResult<typed::LoginAccountResponse> decodeLoginAccountResponseUnion(const Json& value) noexcept;

    std::optional<Json> encodeCancelLoginAccountParams(const typed::CancelLoginAccountParams& params, std::string& error);
    std::optional<Json> encodeLoginAccountParams(const typed::LoginAccountParams& params, std::string& error);
    std::optional<Json> encodeConsumeAccountRateLimitResetCreditParams(const typed::ConsumeAccountRateLimitResetCreditParams& params,
                                                                       std::string& error);
    std::optional<Json> encodeGetAccountParams(const typed::GetAccountParams& params, std::string& error);
    std::optional<Json> encodeSendAddCreditsNudgeEmailParams(const typed::SendAddCreditsNudgeEmailParams& params, std::string& error);

    std::optional<typed::CancelLoginAccountResponse> decodeCancelLoginAccountResponse(const Json& value, std::string& error);
    std::optional<typed::LoginAccountResponse> decodeLoginAccountResponse(const Json& value, std::string& error);
    std::optional<typed::ConsumeAccountRateLimitResetCreditResponse> decodeConsumeAccountRateLimitResetCreditResponse(const Json& value,
                                                                                                                      std::string& error);
    std::optional<typed::GetAccountRateLimitsResponse> decodeGetAccountRateLimitsResponse(const Json& value, std::string& error);
    std::optional<typed::GetAccountResponse> decodeGetAccountResponse(const Json& value, std::string& error);
    std::optional<typed::SendAddCreditsNudgeEmailResponse> decodeSendAddCreditsNudgeEmailResponse(const Json& value, std::string& error);
    std::optional<typed::GetAccountTokenUsageResponse> decodeGetAccountTokenUsageResponse(const Json& value, std::string& error);
    std::optional<typed::GetWorkspaceMessagesResponse> decodeGetWorkspaceMessagesResponse(const Json& value, std::string& error);

    std::optional<typed::AccountLoginCompletedNotification> decodeAccountLoginCompletedNotification(const Notification& notification,
                                                                                                    std::string& error);
    std::optional<typed::AccountRateLimitsUpdatedNotification> decodeAccountRateLimitsUpdatedNotification(const Notification& notification,
                                                                                                          std::string& error);
    std::optional<typed::AccountUpdatedNotification> decodeAccountUpdatedNotification(const Notification& notification, std::string& error);

    std::optional<typed::ChatgptAuthTokensRefreshParams> decodeChatgptAuthTokensRefreshParams(const Json& value, std::string& error);
    std::optional<Json> encodeChatgptAuthTokensRefreshResponse(typed::ChatgptAuthTokensRefreshResponse response, std::string& error);

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_ACCOUNTCODEC_H
