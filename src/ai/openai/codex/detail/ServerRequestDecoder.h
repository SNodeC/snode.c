/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_SERVERREQUESTDECODER_H
#define AI_OPENAI_CODEX_DETAIL_SERVERREQUESTDECODER_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/typed/ServerRequests.h"

namespace ai::openai::codex::detail {

    typed::TypedServerRequest decodeServerRequest(const ServerRequest& request) noexcept;

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_SERVERREQUESTDECODER_H
