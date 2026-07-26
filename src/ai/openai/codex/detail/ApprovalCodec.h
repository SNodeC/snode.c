/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_APPROVALCODEC_H
#define AI_OPENAI_CODEX_DETAIL_APPROVALCODEC_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ConversationCodec.h"
#include "ai/openai/codex/typed/PermissionProfiles.h"
#include "ai/openai/codex/typed/ServerRequests.h"

#include <optional>
#include <string>

namespace ai::openai::codex::detail {

    ConversationDecodeResult<typed::FileChange> decodeFileChange(const Json& value) noexcept;
    ConversationDecodeResult<typed::FileSystemPath> decodeFileSystemPath(const Json& value) noexcept;
    ConversationDecodeResult<typed::FileSystemSpecialPath> decodeFileSystemSpecialPath(const Json& value) noexcept;
    ConversationDecodeResult<typed::ParsedCommand> decodeParsedCommand(const Json& value) noexcept;
    ConversationDecodeResult<typed::CommandExecutionApprovalDecision> decodeCommandExecutionApprovalDecision(const Json& value) noexcept;
    ConversationDecodeResult<typed::ReviewDecision> decodeReviewDecision(const Json& value) noexcept;

    std::optional<Json> encodeFileChange(const typed::FileChange& value, std::string& error) noexcept;
    std::optional<Json> encodeFileSystemPath(const typed::FileSystemPath& value, std::string& error) noexcept;
    std::optional<Json> encodeFileSystemSpecialPath(const typed::FileSystemSpecialPath& value, std::string& error) noexcept;
    std::optional<Json> encodeParsedCommand(const typed::ParsedCommand& value, std::string& error) noexcept;
    std::optional<Json> encodeCommandExecutionApprovalDecision(const typed::CommandExecutionApprovalDecision& value,
                                                               std::string& error) noexcept;
    std::optional<Json> encodeReviewDecision(const typed::ReviewDecision& value, std::string& error) noexcept;

    std::optional<typed::RequestPermissionProfile> decodeRequestPermissionProfile(const Json& value, std::string& error) noexcept;
    std::optional<typed::GrantedPermissionProfile> decodeGrantedPermissionProfile(const Json& value, std::string& error) noexcept;
    std::optional<Json> encodeRequestPermissionProfile(const typed::RequestPermissionProfile& value, std::string& error) noexcept;
    std::optional<Json> encodeGrantedPermissionProfile(const typed::GrantedPermissionProfile& value, std::string& error) noexcept;

    std::optional<typed::ApplyPatchApprovalParams> decodeApplyPatchApprovalParams(const Json& value, std::string& error) noexcept;
    std::optional<typed::ExecCommandApprovalParams> decodeExecCommandApprovalParams(const Json& value, std::string& error) noexcept;
    std::optional<typed::CommandExecutionRequestApprovalParams> decodeCommandExecutionRequestApprovalParams(const Json& value,
                                                                                                            std::string& error) noexcept;
    std::optional<typed::FileChangeRequestApprovalParams> decodeFileChangeRequestApprovalParams(const Json& value,
                                                                                                std::string& error) noexcept;
    std::optional<typed::PermissionsRequestApprovalParams> decodePermissionsRequestApprovalParams(const Json& value,
                                                                                                  std::string& error) noexcept;

    std::optional<Json> encodeApplyPatchApprovalResponse(const typed::ApplyPatchApprovalResponse& value, std::string& error) noexcept;
    std::optional<Json> encodeExecCommandApprovalResponse(const typed::ExecCommandApprovalResponse& value, std::string& error) noexcept;
    std::optional<Json> encodeCommandExecutionRequestApprovalResponse(const typed::CommandExecutionRequestApprovalResponse& value,
                                                                      std::string& error) noexcept;
    std::optional<Json> encodeFileChangeRequestApprovalResponse(const typed::FileChangeRequestApprovalResponse& value,
                                                                std::string& error) noexcept;
    std::optional<Json> encodePermissionsRequestApprovalResponse(const typed::PermissionsRequestApprovalResponse& value,
                                                                 std::string& error) noexcept;

    std::optional<Json> encodePermissionProfileListParams(const typed::PermissionProfileListParams& value, std::string& error) noexcept;
    std::optional<typed::PermissionProfileListResponse> decodePermissionProfileListResponse(const Json& value, std::string& error) noexcept;

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_APPROVALCODEC_H
