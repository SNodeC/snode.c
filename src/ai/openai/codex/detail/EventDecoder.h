/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_EVENTDECODER_H
#define AI_OPENAI_CODEX_DETAIL_EVENTDECODER_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/typed/Events.h"

namespace ai::openai::codex::detail {

    typed::Event decodeEvent(const Notification& notification) noexcept;

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_EVENTDECODER_H
