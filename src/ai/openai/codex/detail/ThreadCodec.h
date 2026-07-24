/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_THREADCODEC_H
#define AI_OPENAI_CODEX_DETAIL_THREADCODEC_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ConversationCodec.h"
#include "ai/openai/codex/typed/Conversation.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/Threads.h"
#include "ai/openai/codex/typed/Types.h"

#include <optional>
#include <string>

namespace ai::openai::codex::detail {

    ConversationDecodeResult<typed::SessionSource> decodeSessionSource(const Json& value) noexcept;
    ConversationDecodeResult<typed::SubAgentSource> decodeSubAgentSource(const Json& value) noexcept;
    ConversationDecodeResult<typed::ThreadStatus> decodeThreadStatus(const Json& value) noexcept;

    std::optional<Json> encodeThreadArchiveParams(const typed::ThreadArchiveParams& params, std::string& error);
    std::optional<Json> encodeThreadCompactStartParams(const typed::ThreadCompactStartParams& params, std::string& error);
    std::optional<Json> encodeThreadDeleteParams(const typed::ThreadDeleteParams& params, std::string& error);
    std::optional<Json> encodeThreadForkParams(const typed::ThreadForkParams& params, std::string& error);
    std::optional<Json> encodeThreadGoalClearParams(const typed::ThreadGoalClearParams& params, std::string& error);
    std::optional<Json> encodeThreadGoalGetParams(const typed::ThreadGoalGetParams& params, std::string& error);
    std::optional<Json> encodeThreadGoalSetParams(const typed::ThreadGoalSetParams& params, std::string& error);
    std::optional<Json> encodeThreadInjectItemsParams(const typed::ThreadInjectItemsParams& params, std::string& error);
    std::optional<Json> encodeThreadListParams(const typed::ThreadListParams& params, std::string& error);
    std::optional<Json> encodeThreadLoadedListParams(const typed::ThreadLoadedListParams& params, std::string& error);
    std::optional<Json> encodeThreadMetadataUpdateParams(const typed::ThreadMetadataUpdateParams& params, std::string& error);
    std::optional<Json> encodeThreadReadParams(const typed::ThreadReadParams& params, std::string& error);
    std::optional<Json> encodeThreadResumeParams(const typed::ThreadResumeParams& params, std::string& error);
    std::optional<Json> encodeThreadRollbackParams(const typed::ThreadRollbackParams& params, std::string& error);
    std::optional<Json> encodeThreadSetNameParams(const typed::ThreadSetNameParams& params, std::string& error);
    std::optional<Json> encodeThreadShellCommandParams(const typed::ThreadShellCommandParams& params, std::string& error);
    std::optional<Json> encodeThreadStartParams(const typed::ThreadStartParams& params, std::string& error);
    std::optional<Json> encodeThreadUnarchiveParams(const typed::ThreadUnarchiveParams& params, std::string& error);
    std::optional<Json> encodeThreadUnsubscribeParams(const typed::ThreadUnsubscribeParams& params, std::string& error);

    std::optional<typed::Unit> decodeUnitResult(const Json& value, std::string& error);
    std::optional<typed::Unit> decodeThreadArchiveResult(const Json& value, std::string& error);
    std::optional<typed::Unit> decodeThreadCompactStartResult(const Json& value, std::string& error);
    std::optional<typed::Unit> decodeThreadDeleteResult(const Json& value, std::string& error);
    std::optional<typed::Unit> decodeThreadInjectItemsResult(const Json& value, std::string& error);
    std::optional<typed::Unit> decodeThreadSetNameResult(const Json& value, std::string& error);
    std::optional<typed::Unit> decodeThreadShellCommandResult(const Json& value, std::string& error);

    std::optional<typed::Thread> decodeThread(const Json& value, std::string& error);
    std::optional<typed::Thread>
    decodeThread(const Json& value, std::string& error, std::optional<typed::ModelId> contextualModel);
    std::optional<typed::ThreadForkResponse> decodeThreadForkResponse(const Json& value, std::string& error);
    std::optional<typed::ThreadGoalClearResponse> decodeThreadGoalClearResponse(const Json& value, std::string& error);
    std::optional<typed::ThreadGoalGetResponse> decodeThreadGoalGetResponse(const Json& value, std::string& error);
    std::optional<typed::ThreadGoalSetResponse> decodeThreadGoalSetResponse(const Json& value, std::string& error);
    std::optional<typed::ThreadListResponse> decodeThreadListResponse(const Json& value, std::string& error);
    std::optional<typed::ThreadLoadedListResponse> decodeThreadLoadedListResponse(const Json& value, std::string& error);
    std::optional<typed::ThreadMetadataUpdateResponse> decodeThreadMetadataUpdateResponse(const Json& value, std::string& error);
    std::optional<typed::ThreadReadResponse> decodeThreadReadResponse(const Json& value, std::string& error);
    std::optional<typed::ThreadResumeResponse> decodeThreadResumeResponse(const Json& value, std::string& error);
    std::optional<typed::ThreadRollbackResponse> decodeThreadRollbackResponse(const Json& value, std::string& error);
    std::optional<typed::ThreadStartResponse> decodeThreadStartResponse(const Json& value, std::string& error);
    std::optional<typed::ThreadUnarchiveResponse> decodeThreadUnarchiveResponse(const Json& value, std::string& error);
    std::optional<typed::ThreadUnsubscribeResponse> decodeThreadUnsubscribeResponse(const Json& value, std::string& error);

    // A1.0 compatibility codec entrypoints.
    std::optional<Json> encodeThreadStartParams(const typed::ThreadStartOptions& options, std::string& error);
    std::optional<Json>
    encodeThreadResumeParams(const typed::ThreadId& threadId, const typed::ThreadResumeOptions& options, std::string& error);
    std::optional<Json> encodeThreadListParams(const typed::ThreadListOptions& options, std::string& error);
    std::optional<Json>
    encodeThreadReadParams(const typed::ThreadId& threadId, const typed::ThreadReadOptions& options, std::string& error);
    std::optional<typed::Thread> decodeThreadOperationResult(const Json& value, std::string& error);
    std::optional<typed::ThreadPage> decodeThreadListResult(const Json& value, std::string& error);
    std::optional<typed::Thread> decodeThreadReadResult(const Json& value, std::string& error);

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_THREADCODEC_H
