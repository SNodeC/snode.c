/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_REVIEWCODEC_H
#define AI_OPENAI_CODEX_DETAIL_REVIEWCODEC_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ConversationCodec.h"
#include "ai/openai/codex/typed/Reviews.h"

#include <optional>
#include <string>

namespace ai::openai::codex::detail {

    ConversationDecodeResult<typed::ReviewTarget> decodeReviewTarget(const Json& value) noexcept;
    ConversationDecodeResult<typed::GuardianApprovalReviewAction> decodeGuardianApprovalReviewAction(const Json& value) noexcept;

    std::optional<Json> encodeReviewTarget(const typed::ReviewTarget& value, std::string& error) noexcept;
    std::optional<Json> encodeReviewStartParams(const typed::ReviewStartParams& value, std::string& error) noexcept;
    std::optional<Json> encodeThreadApproveGuardianDeniedActionParams(const typed::ThreadApproveGuardianDeniedActionParams& value,
                                                                      std::string& error) noexcept;

    std::optional<typed::ReviewStartResponse> decodeReviewStartResponse(const Json& value, std::string& error) noexcept;
    std::optional<typed::GuardianWarningNotification> decodeGuardianWarningNotification(const Notification& notification,
                                                                                        std::string& error) noexcept;
    std::optional<typed::ItemGuardianApprovalReviewStartedNotification>
    decodeItemGuardianApprovalReviewStartedNotification(const Notification& notification, std::string& error) noexcept;
    std::optional<typed::ItemGuardianApprovalReviewCompletedNotification>
    decodeItemGuardianApprovalReviewCompletedNotification(const Notification& notification, std::string& error) noexcept;

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_REVIEWCODEC_H
