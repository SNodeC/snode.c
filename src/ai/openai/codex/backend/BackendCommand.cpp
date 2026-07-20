/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/backend/BackendCommand.h"

#include <utility>

namespace ai::openai::codex::backend {

    CommandResult CommandResult::succeeded(CommandValue value) {
        CommandResult result;
        result.value = std::move(value);
        return result;
    }

    CommandResult CommandResult::failed(CommandErrorCode code, std::string message, std::optional<std::int64_t> remoteCode, Json details) {
        CommandResult result;
        result.error = CommandError{code, std::move(message), remoteCode, std::move(details)};
        return result;
    }

    const char* commandErrorCodeName(CommandErrorCode code) noexcept {
        switch (code) {
            case CommandErrorCode::PermissionDenied:
                return "permission_denied";
            case CommandErrorCode::InvalidCommand:
                return "invalid_command";
            case CommandErrorCode::NotFound:
                return "not_found";
            case CommandErrorCode::Conflict:
                return "conflict";
            case CommandErrorCode::LocalSubmissionFailure:
                return "local_submission_failure";
            case CommandErrorCode::TypedDecodingFailure:
                return "typed_decoding_failure";
            case CommandErrorCode::RemoteAppServerError:
                return "remote_app_server_error";
            case CommandErrorCode::Cancelled:
                return "cancelled";
            case CommandErrorCode::BackendUnavailable:
                return "backend_unavailable";
        }
        return "invalid_command";
    }

} // namespace ai::openai::codex::backend
