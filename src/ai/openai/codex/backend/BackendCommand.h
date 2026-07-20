/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_BACKEND_BACKENDCOMMAND_H
#define AI_OPENAI_CODEX_BACKEND_BACKENDCOMMAND_H

#include "ai/openai/codex/backend/Snapshot.h"

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace ai::openai::codex::backend {

    struct ControllerAcquire {};
    struct ControllerRelease {};
    struct SnapshotGet {};
    struct ReplayAfter {
        SequenceNumber sequence;
    };

    struct ThreadStart {
        typed::ThreadStartOptions options;
    };

    struct ThreadResume {
        typed::ThreadId threadId;
        typed::ThreadResumeOptions options;
    };

    struct ThreadList {
        typed::ThreadListOptions options;
    };

    struct ThreadRead {
        typed::ThreadId threadId;
        typed::ThreadReadOptions options;
    };

    struct TurnStart {
        typed::ThreadId threadId;
        std::vector<typed::TurnInput> input;
        typed::TurnStartOptions options;
    };

    struct TurnInterrupt {
        typed::ThreadId threadId;
        typed::TurnId turnId;
    };

    struct ApprovalRespond {
        PendingRequestId requestId;
        typed::ApprovalDecision decision;
    };

    struct UserInputRespond {
        PendingRequestId requestId;
        std::vector<typed::UserInputAnswer> answers;
    };

    struct AuthenticationRespond {
        PendingRequestId requestId;
        typed::AuthenticationResponse response;
    };

    struct UnknownRequestRespondRaw {
        PendingRequestId requestId;
        Json result = nullptr;
    };

    struct UnknownRequestReject {
        PendingRequestId requestId;
        ProtocolError error;
    };

    using BackendCommand = std::variant<ControllerAcquire,
                                        ControllerRelease,
                                        SnapshotGet,
                                        ReplayAfter,
                                        ThreadStart,
                                        ThreadResume,
                                        ThreadList,
                                        ThreadRead,
                                        TurnStart,
                                        TurnInterrupt,
                                        ApprovalRespond,
                                        UserInputRespond,
                                        AuthenticationRespond,
                                        UnknownRequestRespondRaw,
                                        UnknownRequestReject>;

    enum class CommandErrorCode {
        PermissionDenied,
        InvalidCommand,
        NotFound,
        Conflict,
        LocalSubmissionFailure,
        TypedDecodingFailure,
        RemoteAppServerError,
        Cancelled,
        BackendUnavailable
    };

    struct CommandError {
        CommandErrorCode code = CommandErrorCode::InvalidCommand;
        std::string message;
        std::optional<std::int64_t> remoteCode;
        Json details = nullptr;
    };

    struct ControllerResult {
        std::optional<SessionId> controller;
        SessionRole role = SessionRole::Observer;
    };

    struct ReplayResult {
        SequenceNumber after;
    };

    using CommandValue = std::variant<std::monostate,
                                      Snapshot,
                                      ControllerResult,
                                      ReplayResult,
                                      typed::Thread,
                                      typed::ThreadPage,
                                      typed::Turn,
                                      typed::TurnInterruptResult>;

    struct CommandResult {
        CommandValue value;
        std::optional<CommandError> error;

        explicit operator bool() const noexcept {
            return !error.has_value();
        }

        static CommandResult succeeded(CommandValue value = std::monostate{});
        static CommandResult
        failed(CommandErrorCode code, std::string message, std::optional<std::int64_t> remoteCode = std::nullopt, Json details = nullptr);
    };

    struct CommandCompletion {
        std::string requestId;
        CommandResult result;
    };

    const char* commandErrorCodeName(CommandErrorCode code) noexcept;

} // namespace ai::openai::codex::backend

#endif // AI_OPENAI_CODEX_BACKEND_BACKENDCOMMAND_H
