/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_THREADCODEC_H
#define AI_OPENAI_CODEX_DETAIL_THREADCODEC_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/typed/Threads.h"
#include "ai/openai/codex/typed/Types.h"

#include <optional>
#include <string>

namespace ai::openai::codex::detail {

    std::optional<Json> encodeThreadStartParams(const typed::ThreadStartOptions& options, std::string& error);
    std::optional<Json>
    encodeThreadResumeParams(const typed::ThreadId& threadId, const typed::ThreadResumeOptions& options, std::string& error);
    std::optional<Json> encodeThreadListParams(const typed::ThreadListOptions& options, std::string& error);
    std::optional<Json>
    encodeThreadReadParams(const typed::ThreadId& threadId, const typed::ThreadReadOptions& options, std::string& error);

    std::optional<typed::Thread> decodeThread(const Json& value, std::string& error, std::optional<typed::ModelId> model = std::nullopt);
    std::optional<typed::Thread> decodeThreadOperationResult(const Json& value, std::string& error);
    std::optional<typed::ThreadPage> decodeThreadListResult(const Json& value, std::string& error);
    std::optional<typed::Thread> decodeThreadReadResult(const Json& value, std::string& error);

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_THREADCODEC_H
