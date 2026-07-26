/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_FILESYSTEMCODEC_H
#define AI_OPENAI_CODEX_DETAIL_FILESYSTEMCODEC_H

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/typed/Filesystem.h"

#include <optional>
#include <string>

namespace ai::openai::codex::detail {

    std::optional<Json> encodeFsCopyParams(const typed::FsCopyParams& value, std::string& error) noexcept;
    std::optional<Json> encodeFsCreateDirectoryParams(const typed::FsCreateDirectoryParams& value, std::string& error) noexcept;
    std::optional<Json> encodeFsGetMetadataParams(const typed::FsGetMetadataParams& value, std::string& error) noexcept;
    std::optional<Json> encodeFsReadDirectoryParams(const typed::FsReadDirectoryParams& value, std::string& error) noexcept;
    std::optional<Json> encodeFsReadFileParams(const typed::FsReadFileParams& value, std::string& error) noexcept;
    std::optional<Json> encodeFsRemoveParams(const typed::FsRemoveParams& value, std::string& error) noexcept;
    std::optional<Json> encodeFsUnwatchParams(const typed::FsUnwatchParams& value, std::string& error) noexcept;
    std::optional<Json> encodeFsWatchParams(const typed::FsWatchParams& value, std::string& error) noexcept;
    std::optional<Json> encodeFsWriteFileParams(const typed::FsWriteFileParams& value, std::string& error) noexcept;
    std::optional<Json> encodeFuzzyFileSearchParams(const typed::FuzzyFileSearchParams& value, std::string& error) noexcept;

    std::optional<typed::FsGetMetadataResponse> decodeFsGetMetadataResponse(const Json& value, std::string& error) noexcept;
    std::optional<typed::FsReadDirectoryResponse> decodeFsReadDirectoryResponse(const Json& value, std::string& error) noexcept;
    std::optional<typed::FsReadFileResponse> decodeFsReadFileResponse(const Json& value, std::string& error) noexcept;
    std::optional<typed::FsWatchResponse> decodeFsWatchResponse(const Json& value, std::string& error) noexcept;
    std::optional<typed::FuzzyFileSearchResponse> decodeFuzzyFileSearchResponse(const Json& value, std::string& error) noexcept;

    std::optional<typed::FsChangedNotification> decodeFsChangedNotification(const Notification& notification, std::string& error) noexcept;
    std::optional<typed::FuzzyFileSearchSessionCompletedNotification>
    decodeFuzzyFileSearchSessionCompletedNotification(const Notification& notification, std::string& error) noexcept;
    std::optional<typed::FuzzyFileSearchSessionUpdatedNotification>
    decodeFuzzyFileSearchSessionUpdatedNotification(const Notification& notification, std::string& error) noexcept;

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_FILESYSTEMCODEC_H
