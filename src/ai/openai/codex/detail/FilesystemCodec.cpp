/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/FilesystemCodec.h"

#include "ai/openai/codex/detail/DecodeDiagnostic.h"
#include "ai/openai/codex/typed/Types.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace ai::openai::codex::detail {

    namespace {

        bool fail(std::string& error, std::string message) {
            error = std::move(message);
            return false;
        }

        const Json* member(const Json& object, std::string_view name) noexcept {
            if (!object.is_object()) {
                return nullptr;
            }
            const auto iterator = object.find(name);
            return iterator == object.end() ? nullptr : &*iterator;
        }

        std::string fieldPath(std::string_view base, std::string_view name) {
            std::string result(base.empty() ? "$" : base);
            if (!name.empty()) {
                result += ".";
                result += name;
            }
            return result;
        }

        std::string indexedPath(std::string_view base, std::size_t index) {
            return std::string(base) + "[" + std::to_string(index) + "]";
        }

        bool requireObject(const Json& value, std::string_view context, std::string& error, std::string_view path = "$") {
            return value.is_object() || fail(error, std::string(context) + " at '" + std::string(path) + "' must be an object");
        }

        bool decodeString(const Json& value, std::string& result) {
            if (!value.is_string()) {
                return false;
            }
            result = value.get_ref<const std::string&>();
            return true;
        }

        bool decodeBool(const Json& value, bool& result) {
            if (!value.is_boolean()) {
                return false;
            }
            result = value.get_ref<const Json::boolean_t&>();
            return true;
        }

        bool decodeInt64(const Json& value, std::int64_t& result) {
            if (value.is_number_unsigned()) {
                const auto number = value.get_ref<const Json::number_unsigned_t&>();
                if (number > static_cast<Json::number_unsigned_t>(std::numeric_limits<std::int64_t>::max())) {
                    return false;
                }
                result = static_cast<std::int64_t>(number);
                return true;
            }
            if (!value.is_number_integer()) {
                return false;
            }
            result = value.get_ref<const Json::number_integer_t&>();
            return true;
        }

        bool decodeUint32(const Json& value, std::uint32_t& result) {
            if (value.is_number_unsigned()) {
                const auto number = value.get_ref<const Json::number_unsigned_t&>();
                if (number > std::numeric_limits<std::uint32_t>::max()) {
                    return false;
                }
                result = static_cast<std::uint32_t>(number);
                return true;
            }
            if (!value.is_number_integer()) {
                return false;
            }
            const auto number = value.get_ref<const Json::number_integer_t&>();
            if (number < 0 || static_cast<Json::number_unsigned_t>(number) > std::numeric_limits<std::uint32_t>::max()) {
                return false;
            }
            result = static_cast<std::uint32_t>(number);
            return true;
        }

        template <typename Strong>
        bool decodeStrongString(const Json& value, Strong& result) {
            return decodeString(value, result.value);
        }

        template <typename T, typename Decode>
        bool decodeRequired(const Json& object,
                            std::string_view name,
                            T& result,
                            Decode&& decode,
                            std::string& error,
                            std::string_view context,
                            std::string_view path = "$") {
            const Json* value = member(object, name);
            if (value == nullptr || !decode(*value, result)) {
                if (!error.empty()) {
                    return false;
                }
                return fail(error, std::string(context) + " field '" + fieldPath(path, name) + "' has the wrong type or is missing");
            }
            return true;
        }

        template <typename T, typename Decode>
        bool decodeOptionalNullable(const Json& object,
                                    std::string_view name,
                                    typed::OptionalNullable<T>& result,
                                    Decode&& decode,
                                    std::string& error,
                                    std::string_view context,
                                    std::string_view path = "$") {
            result = typed::OptionalNullable<T>::omitted();
            const Json* value = member(object, name);
            if (value == nullptr) {
                return true;
            }
            if (value->is_null()) {
                result = typed::OptionalNullable<T>::explicitNull();
                return true;
            }
            T decoded;
            if (!decode(*value, decoded)) {
                if (!error.empty()) {
                    return false;
                }
                return fail(error, std::string(context) + " field '" + fieldPath(path, name) + "' has the wrong type");
            }
            result = typed::OptionalNullable<T>::withValue(std::move(decoded));
            return true;
        }

        template <typename T, typename Decode>
        bool decodeArray(const Json& value,
                         std::vector<T>& result,
                         Decode&& decode,
                         std::string& error,
                         std::string_view context,
                         std::string_view path) {
            if (!value.is_array()) {
                return fail(error, std::string(context) + " field '" + std::string(path) + "' must be an array");
            }
            result.clear();
            result.reserve(value.size());
            for (std::size_t index = 0; index < value.size(); ++index) {
                T decoded;
                const std::string itemPath = indexedPath(path, index);
                if (!decode(value[index], decoded, itemPath)) {
                    if (!error.empty()) {
                        return false;
                    }
                    return fail(error, std::string(context) + " field '" + itemPath + "' has the wrong type");
                }
                result.emplace_back(std::move(decoded));
            }
            return true;
        }

        template <typename T>
        bool encodeOptionalNullable(
            Json& object, std::string_view name, const typed::OptionalNullable<T>& value, std::string& error, std::string_view context) {
            if (!value.present && value.value.has_value()) {
                return fail(error, std::string(context) + " field '$." + std::string(name) + "' has an inconsistent omitted state");
            }
            if (value.isOmitted()) {
                return true;
            }
            object[std::string(name)] = value.isNull() ? Json(nullptr) : Json(*value);
            return true;
        }

        void appendDiagnostics(std::vector<typed::DecodeDiagnostic>& target, const std::vector<typed::DecodeDiagnostic>& source) {
            target.insert(target.end(), source.begin(), source.end());
        }

        bool decodeFsReadDirectoryEntry(const Json& value, typed::FsReadDirectoryEntry& result, std::string& error, std::string_view path) {
            if (!requireObject(value, "FsReadDirectoryEntry", error, path) ||
                !decodeRequired(value, "fileName", result.fileName, decodeString, error, "FsReadDirectoryEntry", path) ||
                !decodeRequired(value, "isDirectory", result.isDirectory, decodeBool, error, "FsReadDirectoryEntry", path) ||
                !decodeRequired(value, "isFile", result.isFile, decodeBool, error, "FsReadDirectoryEntry", path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeUint32Array(const Json& value, std::vector<std::uint32_t>& result, std::string& error, std::string_view path) {
            return decodeArray(
                value,
                result,
                [](const Json& item, std::uint32_t& decoded, const std::string&) {
                    return decodeUint32(item, decoded);
                },
                error,
                "FuzzyFileSearchResult",
                path);
        }

        bool
        decodeFuzzyFileSearchResult(const Json& value, typed::FuzzyFileSearchResult& result, std::string& error, std::string_view path) {
            if (!requireObject(value, "FuzzyFileSearchResult", error, path) ||
                !decodeRequired(value, "file_name", result.fileName, decodeString, error, "FuzzyFileSearchResult", path) ||
                !decodeOptionalNullable(
                    value,
                    "indices",
                    result.indices,
                    [&](const Json& input, std::vector<std::uint32_t>& decoded) {
                        return decodeUint32Array(input, decoded, error, fieldPath(path, "indices"));
                    },
                    error,
                    "FuzzyFileSearchResult",
                    path)) {
                return false;
            }

            const Json* matchType = member(value, "match_type");
            const std::string matchTypePath = fieldPath(path, "match_type");
            if (matchType == nullptr || !decodeString(*matchType, result.matchType.value)) {
                return fail(error, "FuzzyFileSearchResult field '" + matchTypePath + "' has the wrong type or is missing");
            }
            if (!result.matchType.isKnown()) {
                result.diagnostics.emplace_back(unknownEnumDiagnostic("FuzzyFileSearchMatchType", matchTypePath));
            }

            if (!decodeRequired(value, "path", result.path, decodeString, error, "FuzzyFileSearchResult", path) ||
                !decodeRequired(value, "root", result.root, decodeString, error, "FuzzyFileSearchResult", path) ||
                !decodeRequired(value, "score", result.score, decodeUint32, error, "FuzzyFileSearchResult", path)) {
                return false;
            }
            result.raw = value;
            return true;
        }

        bool decodeFuzzyFileSearchResults(const Json& value,
                                          std::vector<typed::FuzzyFileSearchResult>& result,
                                          std::vector<typed::DecodeDiagnostic>& diagnostics,
                                          std::string& error,
                                          std::string_view context,
                                          std::string_view path) {
            return decodeArray(
                value,
                result,
                [&](const Json& item, typed::FuzzyFileSearchResult& decoded, const std::string& itemPath) {
                    if (!decodeFuzzyFileSearchResult(item, decoded, error, itemPath)) {
                        return false;
                    }
                    appendDiagnostics(diagnostics, decoded.diagnostics);
                    return true;
                },
                error,
                context,
                path);
        }

    } // namespace

    std::optional<Json> encodeFsCopyParams(const typed::FsCopyParams& value, std::string& error) noexcept {
        try {
            Json result{{"destinationPath", value.destinationPath.value}, {"sourcePath", value.sourcePath.value}};
            if (value.recursive) {
                result["recursive"] = *value.recursive;
            }
            error.clear();
            return std::optional<Json>{std::move(result)};
        } catch (...) {
            fail(error, "fs/copy parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeFsCreateDirectoryParams(const typed::FsCreateDirectoryParams& value, std::string& error) noexcept {
        try {
            Json result{{"path", value.path.value}};
            if (!encodeOptionalNullable(result, "recursive", value.recursive, error, "fs/createDirectory parameters")) {
                return std::nullopt;
            }
            error.clear();
            return std::optional<Json>{std::move(result)};
        } catch (...) {
            fail(error, "fs/createDirectory parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeFsGetMetadataParams(const typed::FsGetMetadataParams& value, std::string& error) noexcept {
        try {
            error.clear();
            return std::optional<Json>{Json{{"path", value.path.value}}};
        } catch (...) {
            fail(error, "fs/getMetadata parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeFsReadDirectoryParams(const typed::FsReadDirectoryParams& value, std::string& error) noexcept {
        try {
            error.clear();
            return std::optional<Json>{Json{{"path", value.path.value}}};
        } catch (...) {
            fail(error, "fs/readDirectory parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeFsReadFileParams(const typed::FsReadFileParams& value, std::string& error) noexcept {
        try {
            error.clear();
            return std::optional<Json>{Json{{"path", value.path.value}}};
        } catch (...) {
            fail(error, "fs/readFile parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeFsRemoveParams(const typed::FsRemoveParams& value, std::string& error) noexcept {
        try {
            Json result{{"path", value.path.value}};
            if (!encodeOptionalNullable(result, "force", value.force, error, "fs/remove parameters") ||
                !encodeOptionalNullable(result, "recursive", value.recursive, error, "fs/remove parameters")) {
                return std::nullopt;
            }
            error.clear();
            return std::optional<Json>{std::move(result)};
        } catch (...) {
            fail(error, "fs/remove parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeFsUnwatchParams(const typed::FsUnwatchParams& value, std::string& error) noexcept {
        try {
            error.clear();
            return std::optional<Json>{Json{{"watchId", value.watchId.value}}};
        } catch (...) {
            fail(error, "fs/unwatch parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeFsWatchParams(const typed::FsWatchParams& value, std::string& error) noexcept {
        try {
            error.clear();
            return std::optional<Json>{Json{{"path", value.path.value}, {"watchId", value.watchId.value}}};
        } catch (...) {
            fail(error, "fs/watch parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeFsWriteFileParams(const typed::FsWriteFileParams& value, std::string& error) noexcept {
        try {
            error.clear();
            return std::optional<Json>{Json{{"dataBase64", value.dataBase64}, {"path", value.path.value}}};
        } catch (...) {
            fail(error, "fs/writeFile parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeFuzzyFileSearchParams(const typed::FuzzyFileSearchParams& value, std::string& error) noexcept {
        try {
            Json result{{"query", value.query}, {"roots", value.roots}};
            if (!encodeOptionalNullable(result, "cancellationToken", value.cancellationToken, error, "fuzzyFileSearch parameters")) {
                return std::nullopt;
            }
            error.clear();
            return std::optional<Json>{std::move(result)};
        } catch (...) {
            fail(error, "fuzzyFileSearch parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<typed::FsGetMetadataResponse> decodeFsGetMetadataResponse(const Json& value, std::string& error) noexcept {
        try {
            error.clear();
            typed::FsGetMetadataResponse result;
            if (!requireObject(value, "FsGetMetadataResponse", error) ||
                !decodeRequired(value, "createdAtMs", result.createdAtMs, decodeInt64, error, "FsGetMetadataResponse") ||
                !decodeRequired(value, "isDirectory", result.isDirectory, decodeBool, error, "FsGetMetadataResponse") ||
                !decodeRequired(value, "isFile", result.isFile, decodeBool, error, "FsGetMetadataResponse") ||
                !decodeRequired(value, "isSymlink", result.isSymlink, decodeBool, error, "FsGetMetadataResponse") ||
                !decodeRequired(value, "modifiedAtMs", result.modifiedAtMs, decodeInt64, error, "FsGetMetadataResponse")) {
                return std::nullopt;
            }
            result.raw = value;
            error.clear();
            return result;
        } catch (...) {
            fail(error, "FsGetMetadataResponse could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::FsReadDirectoryResponse> decodeFsReadDirectoryResponse(const Json& value, std::string& error) noexcept {
        try {
            error.clear();
            typed::FsReadDirectoryResponse result;
            if (!requireObject(value, "FsReadDirectoryResponse", error)) {
                return std::nullopt;
            }
            const Json* entries = member(value, "entries");
            if (entries == nullptr || !decodeArray(
                                          *entries,
                                          result.entries,
                                          [&](const Json& item, typed::FsReadDirectoryEntry& decoded, const std::string& itemPath) {
                                              if (!decodeFsReadDirectoryEntry(item, decoded, error, itemPath)) {
                                                  return false;
                                              }
                                              appendDiagnostics(result.diagnostics, decoded.diagnostics);
                                              return true;
                                          },
                                          error,
                                          "FsReadDirectoryResponse",
                                          "$.entries")) {
                if (entries == nullptr && error.empty()) {
                    fail(error, "FsReadDirectoryResponse field '$.entries' has the wrong type or is missing");
                }
                return std::nullopt;
            }
            result.raw = value;
            error.clear();
            return result;
        } catch (...) {
            fail(error, "FsReadDirectoryResponse could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::FsReadFileResponse> decodeFsReadFileResponse(const Json& value, std::string& error) noexcept {
        try {
            error.clear();
            typed::FsReadFileResponse result;
            if (!requireObject(value, "FsReadFileResponse", error) ||
                !decodeRequired(value, "dataBase64", result.dataBase64, decodeString, error, "FsReadFileResponse")) {
                return std::nullopt;
            }
            result.raw = value;
            error.clear();
            return result;
        } catch (...) {
            fail(error, "FsReadFileResponse could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::FsWatchResponse> decodeFsWatchResponse(const Json& value, std::string& error) noexcept {
        try {
            error.clear();
            typed::FsWatchResponse result;
            if (!requireObject(value, "FsWatchResponse", error) ||
                !decodeRequired(value, "path", result.path, decodeStrongString<typed::AbsolutePathBuf>, error, "FsWatchResponse")) {
                return std::nullopt;
            }
            result.raw = value;
            error.clear();
            return result;
        } catch (...) {
            fail(error, "FsWatchResponse could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::FuzzyFileSearchResponse> decodeFuzzyFileSearchResponse(const Json& value, std::string& error) noexcept {
        try {
            error.clear();
            typed::FuzzyFileSearchResponse result;
            if (!requireObject(value, "FuzzyFileSearchResponse", error)) {
                return std::nullopt;
            }
            const Json* files = member(value, "files");
            if (files == nullptr ||
                !decodeFuzzyFileSearchResults(*files, result.files, result.diagnostics, error, "FuzzyFileSearchResponse", "$.files")) {
                if (files == nullptr && error.empty()) {
                    fail(error, "FuzzyFileSearchResponse field '$.files' has the wrong type or is missing");
                }
                return std::nullopt;
            }
            result.raw = value;
            error.clear();
            return result;
        } catch (...) {
            fail(error, "FuzzyFileSearchResponse could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::FsChangedNotification> decodeFsChangedNotification(const Notification& notification, std::string& error) noexcept {
        try {
            error.clear();
            typed::FsChangedNotification result;
            if (!requireObject(notification.params, "fs/changed params", error, "$.params")) {
                return std::nullopt;
            }
            const Json* changedPaths = member(notification.params, "changedPaths");
            if (changedPaths == nullptr || !decodeArray(
                                               *changedPaths,
                                               result.changedPaths,
                                               [](const Json& item, typed::AbsolutePathBuf& decoded, const std::string&) {
                                                   return decodeStrongString(item, decoded);
                                               },
                                               error,
                                               "fs/changed",
                                               "$.params.changedPaths")) {
                if (changedPaths == nullptr && error.empty()) {
                    fail(error, "fs/changed field '$.params.changedPaths' has the wrong type or is missing");
                }
                return std::nullopt;
            }
            if (!decodeRequired(notification.params,
                                "watchId",
                                result.watchId,
                                decodeStrongString<typed::FsWatchId>,
                                error,
                                "fs/changed",
                                "$.params")) {
                return std::nullopt;
            }
            result.raw = notification.raw;
            error.clear();
            return result;
        } catch (...) {
            fail(error, "fs/changed could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::FuzzyFileSearchSessionCompletedNotification>
    decodeFuzzyFileSearchSessionCompletedNotification(const Notification& notification, std::string& error) noexcept {
        try {
            error.clear();
            typed::FuzzyFileSearchSessionCompletedNotification result;
            if (!requireObject(notification.params, "fuzzyFileSearch/sessionCompleted params", error, "$.params") ||
                !decodeRequired(notification.params,
                                "sessionId",
                                result.sessionId,
                                decodeString,
                                error,
                                "fuzzyFileSearch/sessionCompleted",
                                "$.params")) {
                return std::nullopt;
            }
            result.raw = notification.raw;
            error.clear();
            return result;
        } catch (...) {
            fail(error, "fuzzyFileSearch/sessionCompleted could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::FuzzyFileSearchSessionUpdatedNotification>
    decodeFuzzyFileSearchSessionUpdatedNotification(const Notification& notification, std::string& error) noexcept {
        try {
            error.clear();
            typed::FuzzyFileSearchSessionUpdatedNotification result;
            if (!requireObject(notification.params, "fuzzyFileSearch/sessionUpdated params", error, "$.params")) {
                return std::nullopt;
            }
            const Json* files = member(notification.params, "files");
            if (files == nullptr ||
                !decodeFuzzyFileSearchResults(
                    *files, result.files, result.diagnostics, error, "fuzzyFileSearch/sessionUpdated", "$.params.files")) {
                if (files == nullptr && error.empty()) {
                    fail(error,
                         "fuzzyFileSearch/sessionUpdated field '$.params.files' has the wrong "
                         "type or is missing");
                }
                return std::nullopt;
            }
            if (!decodeRequired(
                    notification.params, "query", result.query, decodeString, error, "fuzzyFileSearch/sessionUpdated", "$.params") ||
                !decodeRequired(notification.params,
                                "sessionId",
                                result.sessionId,
                                decodeString,
                                error,
                                "fuzzyFileSearch/sessionUpdated",
                                "$.params")) {
                return std::nullopt;
            }
            result.raw = notification.raw;
            error.clear();
            return result;
        } catch (...) {
            fail(error, "fuzzyFileSearch/sessionUpdated could not be decoded");
            return std::nullopt;
        }
    }

} // namespace ai::openai::codex::detail
