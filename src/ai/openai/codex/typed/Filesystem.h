/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_TYPED_FILESYSTEM_H
#define AI_OPENAI_CODEX_TYPED_FILESYSTEM_H

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/Types.h"

#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace ai::openai::codex::typed {

    struct FsWatchId {
        std::string value;

        auto operator<=>(const FsWatchId&) const = default;
    };

    struct FuzzyFileSearchMatchType {
        std::string value;

        static FuzzyFileSearchMatchType file() {
            return {"file"};
        }

        static FuzzyFileSearchMatchType directory() {
            return {"directory"};
        }

        [[nodiscard]] bool isKnown() const noexcept {
            return value == "file" || value == "directory";
        }

        auto operator<=>(const FuzzyFileSearchMatchType&) const = default;
    };

    struct FsCopyParams {
        AbsolutePathBuf destinationPath;
        std::optional<bool> recursive;
        AbsolutePathBuf sourcePath;

        bool operator==(const FsCopyParams&) const = default;
    };

    struct FsCreateDirectoryParams {
        AbsolutePathBuf path;
        OptionalNullable<bool> recursive;

        bool operator==(const FsCreateDirectoryParams&) const = default;
    };

    struct FsGetMetadataParams {
        AbsolutePathBuf path;

        auto operator<=>(const FsGetMetadataParams&) const = default;
    };

    struct FsGetMetadataResponse {
        std::int64_t createdAtMs = 0;
        bool isDirectory = false;
        bool isFile = false;
        bool isSymlink = false;
        std::int64_t modifiedAtMs = 0;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const FsGetMetadataResponse&) const = default;
    };

    struct FsReadDirectoryParams {
        AbsolutePathBuf path;

        auto operator<=>(const FsReadDirectoryParams&) const = default;
    };

    struct FsReadDirectoryEntry {
        std::string fileName;
        bool isDirectory = false;
        bool isFile = false;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const FsReadDirectoryEntry&) const = default;
    };

    struct FsReadDirectoryResponse {
        std::vector<FsReadDirectoryEntry> entries;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const FsReadDirectoryResponse&) const = default;
    };

    struct FsReadFileParams {
        AbsolutePathBuf path;

        auto operator<=>(const FsReadFileParams&) const = default;
    };

    struct FsReadFileResponse {
        std::string dataBase64;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const FsReadFileResponse&) const = default;
    };

    struct FsRemoveParams {
        OptionalNullable<bool> force;
        AbsolutePathBuf path;
        OptionalNullable<bool> recursive;

        bool operator==(const FsRemoveParams&) const = default;
    };

    struct FsUnwatchParams {
        FsWatchId watchId;

        auto operator<=>(const FsUnwatchParams&) const = default;
    };

    struct FsWatchParams {
        AbsolutePathBuf path;
        FsWatchId watchId;

        auto operator<=>(const FsWatchParams&) const = default;
    };

    struct FsWatchResponse {
        AbsolutePathBuf path;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const FsWatchResponse&) const = default;
    };

    struct FsWriteFileParams {
        std::string dataBase64;
        AbsolutePathBuf path;

        auto operator<=>(const FsWriteFileParams&) const = default;
    };

    struct FuzzyFileSearchParams {
        OptionalNullable<std::string> cancellationToken;
        std::string query;
        std::vector<std::string> roots;

        bool operator==(const FuzzyFileSearchParams&) const = default;
    };

    struct FuzzyFileSearchResult {
        std::string fileName;
        OptionalNullable<std::vector<std::uint32_t>> indices;
        FuzzyFileSearchMatchType matchType;
        std::string path;
        std::string root;
        std::uint32_t score = 0;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const FuzzyFileSearchResult&) const = default;
    };

    struct FuzzyFileSearchResponse {
        std::vector<FuzzyFileSearchResult> files;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const FuzzyFileSearchResponse&) const = default;
    };

    struct FsChangedNotification {
        std::vector<AbsolutePathBuf> changedPaths;
        FsWatchId watchId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const FsChangedNotification&) const = default;
    };

    struct FuzzyFileSearchSessionCompletedNotification {
        std::string sessionId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const FuzzyFileSearchSessionCompletedNotification&) const = default;
    };

    struct FuzzyFileSearchSessionUpdatedNotification {
        std::vector<FuzzyFileSearchResult> files;
        std::string query;
        std::string sessionId;
        Json raw = Json::object();
        std::vector<DecodeDiagnostic> diagnostics;

        bool operator==(const FuzzyFileSearchSessionUpdatedNotification&) const = default;
    };

    class Filesystem {
    public:
        using Submission = AppServerClient::RawProtocol::Submission;
        using UnitResultHandler = std::function<void(const OperationResult<Unit>&)>;
        using GetMetadataResultHandler = std::function<void(const OperationResult<FsGetMetadataResponse>&)>;
        using ReadDirectoryResultHandler = std::function<void(const OperationResult<FsReadDirectoryResponse>&)>;
        using ReadFileResultHandler = std::function<void(const OperationResult<FsReadFileResponse>&)>;
        using WatchResultHandler = std::function<void(const OperationResult<FsWatchResponse>&)>;
        using FuzzyFileSearchResultHandler = std::function<void(const OperationResult<FuzzyFileSearchResponse>&)>;

        Submission copy(FsCopyParams params, UnitResultHandler handler);
        Submission createDirectory(FsCreateDirectoryParams params, UnitResultHandler handler);
        Submission getMetadata(FsGetMetadataParams params, GetMetadataResultHandler handler);
        Submission readDirectory(FsReadDirectoryParams params, ReadDirectoryResultHandler handler);
        Submission readFile(FsReadFileParams params, ReadFileResultHandler handler);
        Submission remove(FsRemoveParams params, UnitResultHandler handler);
        Submission watch(FsWatchParams params, WatchResultHandler handler);
        Submission unwatch(FsUnwatchParams params, UnitResultHandler handler);
        Submission writeFile(FsWriteFileParams params, UnitResultHandler handler);
        Submission fuzzyFileSearch(FuzzyFileSearchParams params, FuzzyFileSearchResultHandler handler);

    private:
        friend class ::ai::openai::codex::AppServerClient;

        explicit Filesystem(AppServerClient::RawProtocol& protocol) noexcept;

        AppServerClient::RawProtocol* protocol;
    };

} // namespace ai::openai::codex::typed

#endif // AI_OPENAI_CODEX_TYPED_FILESYSTEM_H
