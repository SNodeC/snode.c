/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_COMMANDCODEC_H
#define AI_OPENAI_CODEX_DETAIL_COMMANDCODEC_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/typed/Commands.h"

#include <optional>
#include <string>

namespace ai::openai::codex::detail {

    std::optional<Json> encodeCommandExecParams(const typed::CommandExecParams& value, std::string& error) noexcept;
    std::optional<Json> encodeCommandExecResizeParams(const typed::CommandExecResizeParams& value, std::string& error) noexcept;
    std::optional<Json> encodeCommandExecTerminateParams(const typed::CommandExecTerminateParams& value, std::string& error) noexcept;
    std::optional<Json> encodeCommandExecWriteParams(const typed::CommandExecWriteParams& value, std::string& error) noexcept;

    std::optional<typed::CommandExecResponse> decodeCommandExecResponse(const Json& value, std::string& error) noexcept;
    std::optional<typed::CommandExecOutputDeltaNotification> decodeCommandExecOutputDeltaNotification(const Notification& notification,
                                                                                                      std::string& error) noexcept;

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_COMMANDCODEC_H
