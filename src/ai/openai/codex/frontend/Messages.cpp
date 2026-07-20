/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/frontend/Messages.h"

#include "ai/openai/codex/frontend/Protocol.h"

#include <type_traits>
#include <utility>

namespace ai::openai::codex::frontend {

    std::string_view toString(SessionRole role) noexcept {
        switch (role) {
            case SessionRole::Observer:
                return "observer";
            case SessionRole::Controller:
                return "controller";
        }
        return "observer";
    }

    std::string_view toString(SyncMode mode) noexcept {
        switch (mode) {
            case SyncMode::Replay:
                return "replay";
            case SyncMode::Snapshot:
                return "snapshot";
        }
        return "snapshot";
    }

    std::string_view toString(ErrorCode code) noexcept {
        switch (code) {
            case ErrorCode::PermissionDenied:
                return "permission_denied";
            case ErrorCode::InvalidCommand:
                return "invalid_command";
            case ErrorCode::NotFound:
                return "not_found";
            case ErrorCode::Conflict:
                return "conflict";
            case ErrorCode::LocalSubmissionFailure:
                return "local_submission_failure";
            case ErrorCode::TypedDecodingFailure:
                return "typed_decoding_failure";
            case ErrorCode::RemoteAppServerError:
                return "remote_app_server_error";
            case ErrorCode::Cancelled:
                return "cancelled";
            case ErrorCode::BackendUnavailable:
                return "backend_unavailable";
            case ErrorCode::DuplicateRequestId:
                return "duplicate_request_id";
            case ErrorCode::MalformedJson:
                return "malformed_json";
            case ErrorCode::WrongProtocol:
                return "wrong_protocol";
            case ErrorCode::UnsupportedVersion:
                return "unsupported_version";
            case ErrorCode::MissingField:
                return "missing_field";
            case ErrorCode::InvalidField:
                return "invalid_field";
            case ErrorCode::UnknownKind:
                return "unknown_kind";
            case ErrorCode::UnknownMethod:
                return "unknown_method";
            case ErrorCode::FrameTooLarge:
                return "frame_too_large";
            case ErrorCode::CapacityExceeded:
                return "capacity_exceeded";
            case ErrorCode::SequenceOverflow:
                return "sequence_overflow";
            case ErrorCode::ReplayGap:
                return "replay_gap";
            case ErrorCode::InternalError:
                return "internal_error";
        }
        return "internal_error";
    }

    std::optional<SessionRole> sessionRoleFromString(std::string_view value) noexcept {
        if (value == "observer") {
            return SessionRole::Observer;
        }
        if (value == "controller") {
            return SessionRole::Controller;
        }
        return std::nullopt;
    }

    std::optional<SyncMode> syncModeFromString(std::string_view value) noexcept {
        if (value == "replay") {
            return SyncMode::Replay;
        }
        if (value == "snapshot") {
            return SyncMode::Snapshot;
        }
        return std::nullopt;
    }

    std::optional<ErrorCode> errorCodeFromString(std::string_view value) noexcept {
#define FRONTEND_ERROR_CODE(name, spelling)                                                                                                \
    if (value == spelling) {                                                                                                               \
        return ErrorCode::name;                                                                                                            \
    }
        FRONTEND_ERROR_CODE(PermissionDenied, "permission_denied")
        FRONTEND_ERROR_CODE(InvalidCommand, "invalid_command")
        FRONTEND_ERROR_CODE(NotFound, "not_found")
        FRONTEND_ERROR_CODE(Conflict, "conflict")
        FRONTEND_ERROR_CODE(LocalSubmissionFailure, "local_submission_failure")
        FRONTEND_ERROR_CODE(TypedDecodingFailure, "typed_decoding_failure")
        FRONTEND_ERROR_CODE(RemoteAppServerError, "remote_app_server_error")
        FRONTEND_ERROR_CODE(Cancelled, "cancelled")
        FRONTEND_ERROR_CODE(BackendUnavailable, "backend_unavailable")
        FRONTEND_ERROR_CODE(DuplicateRequestId, "duplicate_request_id")
        FRONTEND_ERROR_CODE(MalformedJson, "malformed_json")
        FRONTEND_ERROR_CODE(WrongProtocol, "wrong_protocol")
        FRONTEND_ERROR_CODE(UnsupportedVersion, "unsupported_version")
        FRONTEND_ERROR_CODE(MissingField, "missing_field")
        FRONTEND_ERROR_CODE(InvalidField, "invalid_field")
        FRONTEND_ERROR_CODE(UnknownKind, "unknown_kind")
        FRONTEND_ERROR_CODE(UnknownMethod, "unknown_method")
        FRONTEND_ERROR_CODE(FrameTooLarge, "frame_too_large")
        FRONTEND_ERROR_CODE(CapacityExceeded, "capacity_exceeded")
        FRONTEND_ERROR_CODE(SequenceOverflow, "sequence_overflow")
        FRONTEND_ERROR_CODE(ReplayGap, "replay_gap")
        FRONTEND_ERROR_CODE(InternalError, "internal_error")
#undef FRONTEND_ERROR_CODE
        return std::nullopt;
    }

    std::string_view toString(CommandMethod methodValue) noexcept {
        switch (methodValue) {
            case CommandMethod::ControllerAcquire:
                return method::ControllerAcquire;
            case CommandMethod::ControllerRelease:
                return method::ControllerRelease;
            case CommandMethod::SnapshotGet:
                return method::SnapshotGet;
            case CommandMethod::EventsReplay:
                return method::EventsReplay;
            case CommandMethod::ThreadStart:
                return method::ThreadStart;
            case CommandMethod::ThreadResume:
                return method::ThreadResume;
            case CommandMethod::ThreadList:
                return method::ThreadList;
            case CommandMethod::ThreadRead:
                return method::ThreadRead;
            case CommandMethod::TurnStart:
                return method::TurnStart;
            case CommandMethod::TurnInterrupt:
                return method::TurnInterrupt;
            case CommandMethod::ApprovalRespond:
                return method::ApprovalRespond;
            case CommandMethod::UserInputRespond:
                return method::UserInputRespond;
            case CommandMethod::AuthenticationRespond:
                return method::AuthenticationRespond;
            case CommandMethod::UnknownRequestRespond:
                return method::UnknownRequestRespond;
            case CommandMethod::UnknownRequestReject:
                return method::UnknownRequestReject;
        }
        return {};
    }

    std::optional<CommandMethod> commandMethodFromString(std::string_view value) noexcept {
#define FRONTEND_COMMAND_METHOD(name, spelling)                                                                                            \
    if (value == spelling) {                                                                                                               \
        return CommandMethod::name;                                                                                                        \
    }
        FRONTEND_COMMAND_METHOD(ControllerAcquire, method::ControllerAcquire)
        FRONTEND_COMMAND_METHOD(ControllerRelease, method::ControllerRelease)
        FRONTEND_COMMAND_METHOD(SnapshotGet, method::SnapshotGet)
        FRONTEND_COMMAND_METHOD(EventsReplay, method::EventsReplay)
        FRONTEND_COMMAND_METHOD(ThreadStart, method::ThreadStart)
        FRONTEND_COMMAND_METHOD(ThreadResume, method::ThreadResume)
        FRONTEND_COMMAND_METHOD(ThreadList, method::ThreadList)
        FRONTEND_COMMAND_METHOD(ThreadRead, method::ThreadRead)
        FRONTEND_COMMAND_METHOD(TurnStart, method::TurnStart)
        FRONTEND_COMMAND_METHOD(TurnInterrupt, method::TurnInterrupt)
        FRONTEND_COMMAND_METHOD(ApprovalRespond, method::ApprovalRespond)
        FRONTEND_COMMAND_METHOD(UserInputRespond, method::UserInputRespond)
        FRONTEND_COMMAND_METHOD(AuthenticationRespond, method::AuthenticationRespond)
        FRONTEND_COMMAND_METHOD(UnknownRequestRespond, method::UnknownRequestRespond)
        FRONTEND_COMMAND_METHOD(UnknownRequestReject, method::UnknownRequestReject)
#undef FRONTEND_COMMAND_METHOD
        return std::nullopt;
    }

    CommandMethod commandMethod(const CommandParameters& parameters) noexcept {
        return std::visit(
            []<typename Parameters>(const Parameters&) noexcept {
                using T = std::remove_cvref_t<Parameters>;
                if constexpr (std::is_same_v<T, ControllerAcquire>) {
                    return CommandMethod::ControllerAcquire;
                } else if constexpr (std::is_same_v<T, ControllerRelease>) {
                    return CommandMethod::ControllerRelease;
                } else if constexpr (std::is_same_v<T, SnapshotGet>) {
                    return CommandMethod::SnapshotGet;
                } else if constexpr (std::is_same_v<T, ReplayAfter>) {
                    return CommandMethod::EventsReplay;
                } else if constexpr (std::is_same_v<T, ThreadStart>) {
                    return CommandMethod::ThreadStart;
                } else if constexpr (std::is_same_v<T, ThreadResume>) {
                    return CommandMethod::ThreadResume;
                } else if constexpr (std::is_same_v<T, ThreadList>) {
                    return CommandMethod::ThreadList;
                } else if constexpr (std::is_same_v<T, ThreadRead>) {
                    return CommandMethod::ThreadRead;
                } else if constexpr (std::is_same_v<T, TurnStart>) {
                    return CommandMethod::TurnStart;
                } else if constexpr (std::is_same_v<T, TurnInterrupt>) {
                    return CommandMethod::TurnInterrupt;
                } else if constexpr (std::is_same_v<T, ApprovalRespond>) {
                    return CommandMethod::ApprovalRespond;
                } else if constexpr (std::is_same_v<T, UserInputRespond>) {
                    return CommandMethod::UserInputRespond;
                } else if constexpr (std::is_same_v<T, AuthenticationRespond>) {
                    return CommandMethod::AuthenticationRespond;
                } else if constexpr (std::is_same_v<T, UnknownRequestRespond>) {
                    return CommandMethod::UnknownRequestRespond;
                } else {
                    return CommandMethod::UnknownRequestReject;
                }
            },
            parameters);
    }

    Response Response::success(std::string requestId, Json result) {
        Response response;
        response.requestId = std::move(requestId);
        response.ok = true;
        response.result = std::move(result);
        return response;
    }

    Response Response::failure(std::string requestId, CommandError error) {
        Response response;
        response.requestId = std::move(requestId);
        response.error = std::move(error);
        return response;
    }

} // namespace ai::openai::codex::frontend
