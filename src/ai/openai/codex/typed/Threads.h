/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_THREADS_H
#define AI_OPENAI_CODEX_TYPED_THREADS_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/Turns.h"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace ai::openai::codex::typed {

    struct Thread {
        ThreadId id;
        std::optional<std::string> title;
        std::optional<std::string> cwd;
        std::optional<ModelId> model;
        std::optional<std::string> modelProvider;
        std::optional<std::string> preview;
        std::optional<std::string> cliVersion;
        std::optional<std::string> sessionId;
        std::optional<ThreadStatus> status;
        std::optional<bool> ephemeral;
        std::optional<std::int64_t> createdAt;
        std::optional<std::int64_t> updatedAt;
        std::vector<Turn> turns;
        Json raw;
    };

    struct ThreadPage {
        std::vector<Thread> data;
        std::optional<std::string> nextCursor;
        std::optional<std::string> backwardsCursor;
        Json raw;
    };

    struct ThreadStartOptions {
        std::optional<std::string> cwd;
        std::optional<ModelId> model;
        std::optional<std::string> modelProvider;
        std::optional<ApprovalPolicy> approvalPolicy;
        std::optional<SandboxMode> sandboxMode;
        std::optional<bool> ephemeral;
    };

    struct ThreadResumeOptions {
        std::optional<std::string> cwd;
        std::optional<ModelId> model;
        std::optional<std::string> modelProvider;
        std::optional<ApprovalPolicy> approvalPolicy;
        std::optional<SandboxMode> sandboxMode;
    };

    struct ThreadListOptions {
        std::optional<std::string> cursor;
        std::optional<std::uint32_t> limit;
        std::optional<bool> archived;
        std::optional<std::string> searchTerm;
    };

    struct ThreadReadOptions {
        std::optional<bool> includeTurns;
    };

    class Threads {
    public:
        using Submission = AppServerClient::RawProtocol::Submission;
        using ThreadResultHandler = std::function<void(const OperationResult<Thread>&)>;
        using ThreadListResultHandler = std::function<void(const OperationResult<ThreadPage>&)>;

        Submission start(ThreadStartOptions options, ThreadResultHandler handler);
        Submission resume(ThreadId threadId, ThreadResumeOptions options, ThreadResultHandler handler);
        Submission list(ThreadListOptions options, ThreadListResultHandler handler);
        Submission read(ThreadId threadId, ThreadResultHandler handler);
        Submission read(ThreadId threadId, ThreadReadOptions options, ThreadResultHandler handler);

    private:
        friend class ::ai::openai::codex::AppServerClient;

        explicit Threads(AppServerClient::RawProtocol& protocol) noexcept;

        AppServerClient::RawProtocol* protocol;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_THREADS_H
