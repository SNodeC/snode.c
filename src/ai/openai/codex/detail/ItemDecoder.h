/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_ITEMDECODER_H
#define AI_OPENAI_CODEX_DETAIL_ITEMDECODER_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/typed/Items.h"
#include "ai/openai/codex/typed/Types.h"

#include <optional>
#include <string>

namespace ai::openai::codex::detail {

    std::optional<typed::Item>
    decodeItem(const Json& value, std::optional<typed::ThreadId> threadId, std::optional<typed::TurnId> turnId, std::string& error);

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_ITEMDECODER_H
