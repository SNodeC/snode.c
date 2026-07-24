/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_CONFIGURATIONCODEC_H
#define AI_OPENAI_CODEX_DETAIL_CONFIGURATIONCODEC_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ConversationCodec.h"
#include "ai/openai/codex/typed/Configuration.h"

#include <optional>
#include <string>

namespace ai::openai::codex::detail {

    ConversationDecodeResult<typed::ConfigLayerSource> decodeConfigLayerSource(const Json& value) noexcept;

    std::optional<Json> encodeConfigReadParams(const typed::ConfigReadParams& params, std::string& error);

    std::optional<typed::ConfigReadResponse> decodeConfigReadResponse(const Json& value, std::string& error);
    std::optional<typed::ConfigRequirementsReadResponse> decodeConfigRequirementsReadResponse(const Json& value, std::string& error);
    std::optional<typed::ConfigWarningNotification> decodeConfigWarningNotification(const Notification& notification, std::string& error);

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_CONFIGURATIONCODEC_H
