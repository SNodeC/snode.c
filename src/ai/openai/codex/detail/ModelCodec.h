/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_MODELCODEC_H
#define AI_OPENAI_CODEX_DETAIL_MODELCODEC_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/typed/Models.h"

#include <optional>
#include <string>

namespace ai::openai::codex::detail {

    std::optional<Json> encodeModelListParams(const typed::ModelListParams& params, std::string& error);
    std::optional<Json>
    encodeModelProviderCapabilitiesReadParams(const typed::ModelProviderCapabilitiesReadParams& params, std::string& error);

    std::optional<typed::ModelListResponse> decodeModelListResponse(const Json& value, std::string& error);
    std::optional<typed::ModelProviderCapabilitiesReadResponse>
    decodeModelProviderCapabilitiesReadResponse(const Json& value, std::string& error);

    std::optional<typed::ModelReroutedNotification> decodeModelReroutedNotification(const Notification& notification,
                                                                                   std::string& error);
    std::optional<typed::ModelSafetyBufferingUpdatedNotification>
    decodeModelSafetyBufferingUpdatedNotification(const Notification& notification, std::string& error);
    std::optional<typed::ModelVerificationNotification> decodeModelVerificationNotification(const Notification& notification,
                                                                                            std::string& error);

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_MODELCODEC_H
