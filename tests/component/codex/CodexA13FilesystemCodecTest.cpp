/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ClientOperationCodec.h"
#include "ai/openai/codex/detail/EventDecoder.h"
#include "ai/openai/codex/detail/FilesystemCodec.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/typed/Events.h"
#include "ai/openai/codex/typed/Filesystem.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/Types.h"
#include "support/TestResult.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    template <typename T>
    concept HasSessionStart = requires { &T::sessionStart; };

    template <typename T>
    concept HasSessionUpdate = requires { &T::sessionUpdate; };

    template <typename T>
    concept HasSessionStop = requires { &T::sessionStop; };

    template <typename T>
    concept HasFuzzySessionStart = requires { &T::fuzzyFileSearchSessionStart; };

    template <typename T>
    concept HasFuzzySessionUpdate = requires { &T::fuzzyFileSearchSessionUpdate; };

    template <typename T>
    concept HasFuzzySessionStop = requires { &T::fuzzyFileSearchSessionStop; };

    static_assert(!HasSessionStart<typed::Filesystem>);
    static_assert(!HasSessionUpdate<typed::Filesystem>);
    static_assert(!HasSessionStop<typed::Filesystem>);
    static_assert(!HasFuzzySessionStart<typed::Filesystem>);
    static_assert(!HasFuzzySessionUpdate<typed::Filesystem>);
    static_assert(!HasFuzzySessionStop<typed::Filesystem>);
    static_assert(std::is_same_v<decltype(typed::FsReadFileParams::path), typed::AbsolutePathBuf>);
    static_assert(std::is_same_v<decltype(typed::FsWriteFileParams::path), typed::AbsolutePathBuf>);
    static_assert(std::is_same_v<decltype(typed::FsWriteFileParams::dataBase64), std::string>);

    bool hasDiagnostic(const std::vector<typed::DecodeDiagnostic>& diagnostics,
                       typed::DecodeIssueKind kind,
                       typed::DecodeIssueSeverity severity,
                       std::string_view surface,
                       std::string_view path) {
        for (const typed::DecodeDiagnostic& diagnostic : diagnostics) {
            if (diagnostic.kind == kind && diagnostic.severity == severity && diagnostic.surface == surface &&
                diagnostic.fieldPath == path) {
                return true;
            }
        }
        return false;
    }

    codex::Notification notification(std::string method, codex::Json params) {
        codex::Json raw{
            {"jsonrpc", "2.0"},
            {"method", method},
            {"params", params},
        };
        return {std::move(method), std::move(params), std::move(raw)};
    }

    codex::Json fuzzyResult(std::string fileName,
                            std::optional<codex::Json> indices,
                            std::string matchType,
                            std::string path,
                            std::string root,
                            codex::Json score) {
        codex::Json result{
            {"file_name", std::move(fileName)},
            {"match_type", std::move(matchType)},
            {"path", std::move(path)},
            {"root", std::move(root)},
            {"score", std::move(score)},
        };
        if (indices) {
            result["indices"] = std::move(*indices);
        }
        return result;
    }

    void testOpenMatchTypesAndExperimentalBoundary(tests::support::TestResult& result) {
        result.expectTrue(typed::FuzzyFileSearchMatchType::file().value == "file" && typed::FuzzyFileSearchMatchType::file().isKnown() &&
                              typed::FuzzyFileSearchMatchType::directory().value == "directory" &&
                              typed::FuzzyFileSearchMatchType::directory().isKnown(),
                          "fuzzy match factories preserve both pinned stable values");
        result.expectTrue(!typed::FuzzyFileSearchMatchType{"future-match"}.isKnown(),
                          "fuzzy match type remains open to future wire values");
        result.expectTrue(!HasSessionStart<typed::Filesystem> && !HasSessionUpdate<typed::Filesystem> &&
                              !HasSessionStop<typed::Filesystem> && !HasFuzzySessionStart<typed::Filesystem> &&
                              !HasFuzzySessionUpdate<typed::Filesystem> && !HasFuzzySessionStop<typed::Filesystem>,
                          "stable Filesystem exposes no experimental fuzzy session control API");
    }

    void testFilesystemRequestEncoding(tests::support::TestResult& result) {
        std::string error = "stale";
        const std::string exactPath = "/synthetic/path with spaces/back\\slash.bin";

        result.expectTrue(detail::encodeFsCopyParams(
                              {
                                  .destinationPath = {"/synthetic/destination"},
                                  .recursive = std::nullopt,
                                  .sourcePath = {"/synthetic/source"},
                              },
                              error) ==
                                  codex::Json{
                                      {"destinationPath", "/synthetic/destination"},
                                      {"sourcePath", "/synthetic/source"},
                                  } &&
                              error.empty(),
                          "fs/copy minimum request omits its optional recursive flag");
        result.expectTrue(detail::encodeFsCopyParams(
                              {
                                  .destinationPath = {"/synthetic/destination-full"},
                                  .recursive = false,
                                  .sourcePath = {exactPath},
                              },
                              error) ==
                              codex::Json{
                                  {"destinationPath", "/synthetic/destination-full"},
                                  {"recursive", false},
                                  {"sourcePath", exactPath},
                              },
                          "fs/copy full request preserves path bytes and present false");

        result.expectTrue(detail::encodeFsCreateDirectoryParams(
                              {
                                  .path = {exactPath},
                                  .recursive = typed::OptionalNullable<bool>::omitted(),
                              },
                              error) == codex::Json{{"path", exactPath}},
                          "fs/createDirectory minimum request preserves path and omission");
        result.expectTrue(detail::encodeFsCreateDirectoryParams(
                              {
                                  .path = {exactPath},
                                  .recursive = typed::OptionalNullable<bool>::explicitNull(),
                              },
                              error) ==
                                  codex::Json{
                                      {"path", exactPath},
                                      {"recursive", nullptr},
                                  } &&
                              detail::encodeFsCreateDirectoryParams(
                                  {
                                      .path = {exactPath},
                                      .recursive = typed::OptionalNullable<bool>::withValue(false),
                                  },
                                  error) ==
                                  codex::Json{
                                      {"path", exactPath},
                                      {"recursive", false},
                                  },
                          "fs/createDirectory distinguishes null, value, and omitted recursive");
        typed::FsCreateDirectoryParams inconsistentCreate{
            .path = {exactPath},
            .recursive = typed::OptionalNullable<bool>::omitted(),
        };
        inconsistentCreate.recursive.value = false;
        const auto rejectedCreate = detail::encodeFsCreateDirectoryParams(inconsistentCreate, error);
        result.expectTrue(!rejectedCreate && error.find("$.recursive") != std::string::npos,
                          "fs/createDirectory rejects an inconsistent omitted recursive state");

        result.expectTrue(detail::encodeFsGetMetadataParams({{exactPath}}, error) == codex::Json{{"path", exactPath}} &&
                              detail::encodeFsReadDirectoryParams({{exactPath}}, error) == codex::Json{{"path", exactPath}} &&
                              detail::encodeFsReadFileParams({{exactPath}}, error) == codex::Json{{"path", exactPath}},
                          "metadata, directory, and read-file requests preserve their sole path field");

        result.expectTrue(detail::encodeFsRemoveParams(
                              {
                                  .force = typed::OptionalNullable<bool>::omitted(),
                                  .path = {exactPath},
                                  .recursive = typed::OptionalNullable<bool>::omitted(),
                              },
                              error) == codex::Json{{"path", exactPath}},
                          "fs/remove minimum request omits both nullable flags");
        result.expectTrue(detail::encodeFsRemoveParams(
                              {
                                  .force = typed::OptionalNullable<bool>::explicitNull(),
                                  .path = {exactPath},
                                  .recursive = typed::OptionalNullable<bool>::withValue(true),
                              },
                              error) ==
                              codex::Json{
                                  {"force", nullptr},
                                  {"path", exactPath},
                                  {"recursive", true},
                              },
                          "fs/remove full request distinguishes explicit null and value");
        result.expectTrue(detail::encodeFsRemoveParams(
                              {
                                  .force = typed::OptionalNullable<bool>::withValue(false),
                                  .path = {exactPath},
                                  .recursive = typed::OptionalNullable<bool>::explicitNull(),
                              },
                              error) ==
                              codex::Json{
                                  {"force", false},
                                  {"path", exactPath},
                                  {"recursive", nullptr},
                              },
                          "fs/remove covers non-null force and explicit-null recursive");
        typed::FsRemoveParams inconsistentRemove{
            .force = typed::OptionalNullable<bool>::omitted(),
            .path = {exactPath},
            .recursive = typed::OptionalNullable<bool>::omitted(),
        };
        inconsistentRemove.force.value = true;
        const auto rejectedRemove = detail::encodeFsRemoveParams(inconsistentRemove, error);
        result.expectTrue(!rejectedRemove && error.find("$.force") != std::string::npos,
                          "fs/remove rejects an inconsistent omitted force state");

        result.expectTrue(detail::encodeFsWatchParams({{exactPath}, {"watch/exact id"}}, error) ==
                                  codex::Json{
                                      {"path", exactPath},
                                      {"watchId", "watch/exact id"},
                                  } &&
                              detail::encodeFsUnwatchParams({{"watch/exact id"}}, error) == codex::Json{{"watchId", "watch/exact id"}},
                          "watch and unwatch correlate the exact caller-supplied watch id");

        result.expectTrue(detail::encodeFsWriteFileParams({"not-canonical_base64==", {exactPath}}, error) ==
                              codex::Json{
                                  {"dataBase64", "not-canonical_base64=="},
                                  {"path", exactPath},
                              },
                          "fs/writeFile preserves encoded content without local decoding or validation");
    }

    void testFuzzyRequestEncoding(tests::support::TestResult& result) {
        std::string error = "stale";
        const typed::FuzzyFileSearchParams minimum{
            .cancellationToken = typed::OptionalNullable<std::string>::omitted(),
            .query = "",
            .roots = {},
        };
        result.expectTrue(detail::encodeFuzzyFileSearchParams(minimum, error) ==
                                  codex::Json{
                                      {"query", ""},
                                      {"roots", codex::Json::array()},
                                  } &&
                              error.empty(),
                          "fuzzyFileSearch minimum request preserves empty query, roots, and omission");

        const typed::FuzzyFileSearchParams nullToken{
            .cancellationToken = typed::OptionalNullable<std::string>::explicitNull(),
            .query = "synthetic query",
            .roots = {"/synthetic/root-b", "/synthetic/root-a"},
        };
        result.expectTrue(detail::encodeFuzzyFileSearchParams(nullToken, error) ==
                              codex::Json{
                                  {"cancellationToken", nullptr},
                                  {"query", "synthetic query"},
                                  {"roots", codex::Json::array({"/synthetic/root-b", "/synthetic/root-a"})},
                              },
                          "fuzzyFileSearch preserves explicit null and caller root ordering");

        const typed::FuzzyFileSearchParams full{
            .cancellationToken = typed::OptionalNullable<std::string>::withValue("cancellation/token"),
            .query = "needle",
            .roots = {"/synthetic/root with spaces", "/synthetic/root\\wire"},
        };
        result.expectTrue(detail::encodeFuzzyFileSearchParams(full, error) ==
                              codex::Json{
                                  {"cancellationToken", "cancellation/token"},
                                  {"query", "needle"},
                                  {"roots", codex::Json::array({"/synthetic/root with spaces", "/synthetic/root\\wire"})},
                              },
                          "fuzzyFileSearch full request preserves token, query, and root bytes");
        typed::FuzzyFileSearchParams inconsistent{
            .cancellationToken = typed::OptionalNullable<std::string>::omitted(),
            .query = "needle",
            .roots = {"/synthetic/root"},
        };
        inconsistent.cancellationToken.value = "must-not-leak";
        const auto rejected = detail::encodeFuzzyFileSearchParams(inconsistent, error);
        result.expectTrue(!rejected && error.find("$.cancellationToken") != std::string::npos &&
                              error.find("must-not-leak") == std::string::npos,
                          "fuzzyFileSearch rejects an inconsistent token without disclosing it");
    }

    void testFilesystemResponseDecoding(tests::support::TestResult& result) {
        std::string error = "stale";
        const codex::Json metadataWire{
            {"createdAtMs", std::numeric_limits<std::int64_t>::min()},
            {"futureMetadata", true},
            {"isDirectory", false},
            {"isFile", true},
            {"isSymlink", false},
            {"modifiedAtMs", std::numeric_limits<std::int64_t>::max()},
        };
        const auto metadata = detail::decodeFsGetMetadataResponse(metadataWire, error);
        result.expectTrue(metadata && error.empty() && metadata->createdAtMs == std::numeric_limits<std::int64_t>::min() &&
                              metadata->modifiedAtMs == std::numeric_limits<std::int64_t>::max() && !metadata->isDirectory &&
                              metadata->isFile && !metadata->isSymlink && metadata->raw == metadataWire,
                          "metadata decodes every stable field at int64 bounds and retains raw");

        const auto aboveInt64 = detail::decodeFsGetMetadataResponse(
            {
                {"createdAtMs", std::numeric_limits<std::uint64_t>::max()},
                {"isDirectory", false},
                {"isFile", true},
                {"isSymlink", false},
                {"modifiedAtMs", 0},
            },
            error);
        result.expectTrue(!aboveInt64 && error.find("$.createdAtMs") != std::string::npos, "metadata rejects an integer above int64");
        error = "stale diagnostic";
        const auto missingMetadata = detail::decodeFsGetMetadataResponse(
            {
                {"createdAtMs", 0},
                {"isDirectory", false},
                {"isFile", true},
                {"modifiedAtMs", 0},
            },
            error);
        result.expectTrue(!missingMetadata && error.find("$.isSymlink") != std::string::npos,
                          "metadata rejects a missing required field with its path");

        const auto emptyDirectory = detail::decodeFsReadDirectoryResponse({{"entries", codex::Json::array()}}, error);
        result.expectTrue(emptyDirectory && emptyDirectory->entries.empty(), "readDirectory preserves an empty entry array");
        const codex::Json directoryWire{
            {"entries",
             codex::Json::array({
                 {
                     {"fileName", "file with spaces.bin"},
                     {"futureEntry", 1},
                     {"isDirectory", false},
                     {"isFile", true},
                 },
                 {
                     {"fileName", "directory"},
                     {"isDirectory", true},
                     {"isFile", false},
                 },
             })},
            {"futureResponse", true},
        };
        const auto directory = detail::decodeFsReadDirectoryResponse(directoryWire, error);
        result.expectTrue(directory && directory->entries.size() == 2 && directory->entries[0].fileName == "file with spaces.bin" &&
                              directory->entries[0].isFile && directory->entries[0].raw.at("futureEntry") == 1 &&
                              directory->entries[1].isDirectory && directory->raw == directoryWire,
                          "readDirectory preserves entry ordering, flags, names, and raw extensions");
        const auto malformedDirectory = detail::decodeFsReadDirectoryResponse(
            {{"entries", codex::Json::array({{{"fileName", 7}, {"isDirectory", false}, {"isFile", true}}})}}, error);
        result.expectTrue(!malformedDirectory && error.find("$.entries[0].fileName") != std::string::npos,
                          "readDirectory rejects a wrong nested entry scalar type");

        const codex::Json fileWire{
            {"dataBase64", "not-canonical_base64=="},
            {"futureReadField", true},
        };
        const auto file = detail::decodeFsReadFileResponse(fileWire, error);
        result.expectTrue(file && file->dataBase64 == "not-canonical_base64==" && file->raw == fileWire,
                          "readFile preserves encoded bytes without decoding or re-encoding");
        result.expectTrue(!detail::decodeFsReadFileResponse({{"dataBase64", 7}}, error) && error.find("$.dataBase64") != std::string::npos,
                          "readFile rejects a wrong encoded-content scalar type");

        const codex::Json watchWire{
            {"path", "/synthetic/canonical path/back\\slash"},
            {"futureWatchField", true},
        };
        const auto watch = detail::decodeFsWatchResponse(watchWire, error);
        result.expectTrue(watch && watch->path.value == "/synthetic/canonical path/back\\slash" && watch->raw == watchWire,
                          "watch response preserves the server path exactly");
        result.expectTrue(!detail::decodeFsWatchResponse(codex::Json::object(), error) && error.find("$.path") != std::string::npos,
                          "watch response rejects a missing path");
    }

    void testFuzzyResponseDecoding(tests::support::TestResult& result) {
        std::string error = "stale";
        const codex::Json first = fuzzyResult("first.cpp",
                                              codex::Json::array({0, std::numeric_limits<std::uint32_t>::max()}),
                                              "file",
                                              "src/first.cpp",
                                              "/synthetic/root-b",
                                              std::numeric_limits<std::uint32_t>::max());
        codex::Json second = fuzzyResult("second", codex::Json(nullptr), "future-match", "second", "/synthetic/root-a", 0);
        second["futureResultField"] = true;
        const codex::Json wire{
            {"files", codex::Json::array({first, second})},
            {"futureResponseField", true},
        };
        const auto decoded = detail::decodeFuzzyFileSearchResponse(wire, error);
        result.expectTrue(
            decoded && error.empty() && decoded->files.size() == 2 && decoded->files[0].fileName == "first.cpp" &&
                decoded->files[0].indices.hasValue() && decoded->files[0].indices->at(1) == std::numeric_limits<std::uint32_t>::max() &&
                decoded->files[0].score == std::numeric_limits<std::uint32_t>::max() && decoded->files[1].fileName == "second" &&
                decoded->files[1].indices.isNull() && decoded->files[1].matchType.value == "future-match" &&
                !decoded->files[1].matchType.isKnown() && decoded->files[1].raw.at("futureResultField") == true && decoded->raw == wire,
            "fuzzy response preserves ordering, nullable indices, uint32 bounds, "
            "open values, paths, and raw");
        result.expectTrue(decoded && hasDiagnostic(decoded->diagnostics,
                                                   typed::DecodeIssueKind::UnknownEnumValue,
                                                   typed::DecodeIssueSeverity::ForwardCompatibility,
                                                   "FuzzyFileSearchMatchType",
                                                   "$.files[1].match_type"),
                          "future fuzzy match values retain a forward-compatibility diagnostic");

        codex::Json omittedIndices = fuzzyResult("omitted", std::nullopt, "directory", "dir", "/root", 1);
        const auto omitted = detail::decodeFuzzyFileSearchResponse({{"files", codex::Json::array({omittedIndices})}}, error);
        result.expectTrue(omitted && omitted->files[0].indices.isOmitted() &&
                              omitted->files[0].matchType == typed::FuzzyFileSearchMatchType::directory(),
                          "fuzzy result distinguishes omitted indices and known directory");
        const codex::Json emptyIndices = fuzzyResult("empty", codex::Json::array(), "file", "empty", "/root", 1);
        const auto empty = detail::decodeFuzzyFileSearchResponse({{"files", codex::Json::array({emptyIndices})}}, error);
        result.expectTrue(empty && empty->files[0].indices.hasValue() && empty->files[0].indices->empty(),
                          "fuzzy result preserves a present empty indices array");

        const auto below = detail::decodeFuzzyFileSearchResponse(
            {{"files", codex::Json::array({fuzzyResult("bad", codex::Json::array(), "file", "bad", "/root", -1)})}}, error);
        result.expectTrue(!below && error.find("$.files[0].score") != std::string::npos, "fuzzy result rejects a negative uint32 score");
        const auto above = detail::decodeFuzzyFileSearchResponse(
            {{"files",
              codex::Json::array(
                  {fuzzyResult("bad",
                               codex::Json::array({static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) + 1U}),
                               "file",
                               "bad",
                               "/root",
                               1)})}},
            error);
        result.expectTrue(!above && error.find("$.files[0].indices[0]") != std::string::npos, "fuzzy result rejects an index above uint32");
        const auto malformedKnown = detail::decodeFuzzyFileSearchResponse(
            {{"files", codex::Json::array({fuzzyResult("bad", codex::Json::array(), "file", "bad", "/root", 1)})}}, error);
        result.expectTrue(malformedKnown.has_value(), "known fuzzy file alternative decodes when structurally valid");
        codex::Json wrongMatch = fuzzyResult("bad", codex::Json::array(), "file", "bad", "/root", 1);
        wrongMatch["match_type"] = codex::Json::object();
        result.expectTrue(!detail::decodeFuzzyFileSearchResponse({{"files", codex::Json::array({std::move(wrongMatch)})}}, error) &&
                              error.find("$.files[0].match_type") != std::string::npos,
                          "malformed known fuzzy result is rejected rather than treated as a future value");
    }

    void testNotificationDecoding(tests::support::TestResult& result) {
        std::string error = "stale";
        const codex::Notification changed =
            notification("fs/changed",
                         {
                             {"changedPaths", codex::Json::array({"/synthetic/watch/a", "/synthetic/watch/path with spaces/back\\slash"})},
                             {"futureParam", true},
                             {"watchId", "watch/correlation"},
                         });
        const auto changedDecoded = detail::decodeFsChangedNotification(changed, error);
        result.expectTrue(changedDecoded && error.empty() && changedDecoded->watchId.value == "watch/correlation" &&
                              changedDecoded->changedPaths.size() == 2 &&
                              changedDecoded->changedPaths[1].value == "/synthetic/watch/path with spaces/back\\slash" &&
                              changedDecoded->raw == changed.raw,
                          "fs/changed preserves watch correlation, path order, bytes, and raw");
        const auto emptyChanged = detail::decodeFsChangedNotification(notification("fs/changed",
                                                                                   {
                                                                                       {"changedPaths", codex::Json::array()},
                                                                                       {"watchId", "watch/empty"},
                                                                                   }),
                                                                      error);
        result.expectTrue(emptyChanged && emptyChanged->changedPaths.empty(), "fs/changed preserves an empty path array");

        const codex::Notification completed =
            notification("fuzzyFileSearch/sessionCompleted", {{"futureParam", true}, {"sessionId", "stable/session"}});
        const auto completedDecoded = detail::decodeFuzzyFileSearchSessionCompletedNotification(completed, error);
        result.expectTrue(completedDecoded && completedDecoded->sessionId == "stable/session" && completedDecoded->raw == completed.raw,
                          "stable fuzzy completion notification preserves session identity and raw");

        const codex::Json updatedFuture = fuzzyResult("future", codex::Json(nullptr), "future-match", "future/path", "/synthetic/root", 9);
        const codex::Notification updated = notification("fuzzyFileSearch/sessionUpdated",
                                                         {
                                                             {"files", codex::Json::array({updatedFuture})},
                                                             {"query", "synthetic query"},
                                                             {"sessionId", "stable/session"},
                                                         });
        const auto updatedDecoded = detail::decodeFuzzyFileSearchSessionUpdatedNotification(updated, error);
        result.expectTrue(updatedDecoded && updatedDecoded->files.size() == 1 && updatedDecoded->files[0].path == "future/path" &&
                              updatedDecoded->query == "synthetic query" && updatedDecoded->sessionId == "stable/session" &&
                              updatedDecoded->raw == updated.raw &&
                              hasDiagnostic(updatedDecoded->diagnostics,
                                            typed::DecodeIssueKind::UnknownEnumValue,
                                            typed::DecodeIssueSeverity::ForwardCompatibility,
                                            "FuzzyFileSearchMatchType",
                                            "$.params.files[0].match_type"),
                          "stable fuzzy update preserves fields and diagnoses a future match value");
        const auto emptyUpdated =
            detail::decodeFuzzyFileSearchSessionUpdatedNotification(notification("fuzzyFileSearch/sessionUpdated",
                                                                                 {
                                                                                     {"files", codex::Json::array()},
                                                                                     {"query", ""},
                                                                                     {"sessionId", "stable/empty-session"},
                                                                                 }),
                                                                    error);
        result.expectTrue(emptyUpdated && emptyUpdated->files.empty() && emptyUpdated->query.empty() &&
                              emptyUpdated->sessionId == "stable/empty-session",
                          "stable fuzzy update preserves an empty files array and query");

        result.expectTrue(std::holds_alternative<typed::FsChangedNotification>(detail::decodeEvent(changed)) &&
                              std::holds_alternative<typed::FuzzyFileSearchSessionCompletedNotification>(detail::decodeEvent(completed)) &&
                              std::holds_alternative<typed::FuzzyFileSearchSessionUpdatedNotification>(detail::decodeEvent(updated)),
                          "all three filesystem/fuzzy notifications use the existing Event variant");

        const codex::Notification malformed = notification("fs/changed",
                                                           {
                                                               {"changedPaths", codex::Json::array({7})},
                                                               {"watchId", "watch/correlation"},
                                                           });
        const typed::Event malformedEvent = detail::decodeEvent(malformed);
        const auto* unknown = std::get_if<typed::UnknownEvent>(&malformedEvent);
        result.expectTrue(unknown && unknown->method == "fs/changed" && unknown->diagnostic &&
                              unknown->diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                              unknown->diagnostic->severity == typed::DecodeIssueSeverity::ProtocolWarning &&
                              unknown->raw.at("params").at("changedPaths").at(0) == 7,
                          "malformed known fs/changed remains observable as a protocol warning");

        const auto malformedCompleted = detail::decodeFuzzyFileSearchSessionCompletedNotification(
            notification("fuzzyFileSearch/sessionCompleted", {{"sessionId", 7}}), error);
        result.expectTrue(!malformedCompleted && error.find("$.params.sessionId") != std::string::npos,
                          "fuzzy session completion rejects a wrong session id type");
    }

    void testOperationAssociations(tests::support::TestResult& result) {
        const codex::Json metadata{
            {"createdAtMs", 1},
            {"isDirectory", false},
            {"isFile", true},
            {"isSymlink", false},
            {"modifiedAtMs", 2},
        };
        const codex::Json directory{{"entries", codex::Json::array()}};
        const codex::Json file{{"dataBase64", "c3ludGhldGlj"}};
        const codex::Json watch{{"path", "/synthetic/watch"}};
        const codex::Json fuzzy{{"files", codex::Json::array()}};

        const auto copy = detail::decodeClientOperationResult(detail::ClientRequestTarget::FsCopy, codex::Json::object());
        const auto create = detail::decodeClientOperationResult(detail::ClientRequestTarget::FsCreateDirectory, codex::Json::object());
        const auto getMetadata = detail::decodeClientOperationResult(detail::ClientRequestTarget::FsGetMetadata, metadata);
        const auto readDirectory = detail::decodeClientOperationResult(detail::ClientRequestTarget::FsReadDirectory, directory);
        const auto readFile = detail::decodeClientOperationResult(detail::ClientRequestTarget::FsReadFile, file);
        const auto remove = detail::decodeClientOperationResult(detail::ClientRequestTarget::FsRemove, codex::Json::object());
        const auto unwatch = detail::decodeClientOperationResult(detail::ClientRequestTarget::FsUnwatch, codex::Json::object());
        const auto watchResult = detail::decodeClientOperationResult(detail::ClientRequestTarget::FsWatch, watch);
        const auto write = detail::decodeClientOperationResult(detail::ClientRequestTarget::FsWriteFile, codex::Json::object());
        const auto fuzzyResult = detail::decodeClientOperationResult(detail::ClientRequestTarget::FuzzyFileSearch, fuzzy);

        result.expectTrue(
            copy && std::holds_alternative<typed::Unit>(*copy.value) && create && std::holds_alternative<typed::Unit>(*create.value) &&
                getMetadata && std::holds_alternative<typed::FsGetMetadataResponse>(*getMetadata.value) && readDirectory &&
                std::holds_alternative<typed::FsReadDirectoryResponse>(*readDirectory.value) && readFile &&
                std::holds_alternative<typed::FsReadFileResponse>(*readFile.value) && remove &&
                std::holds_alternative<typed::Unit>(*remove.value) && unwatch && std::holds_alternative<typed::Unit>(*unwatch.value) &&
                watchResult && std::holds_alternative<typed::FsWatchResponse>(*watchResult.value) && write &&
                std::holds_alternative<typed::Unit>(*write.value) && fuzzyResult &&
                std::holds_alternative<typed::FuzzyFileSearchResponse>(*fuzzyResult.value),
            "all ten targets select their five concrete and five Unit result contracts");

        const auto malformedUnit = detail::decodeClientOperationResult(detail::ClientRequestTarget::FsWriteFile, {{"unexpected", true}});
        result.expectTrue(!malformedUnit && malformedUnit.diagnostic.code == detail::ClientOperationDecodeCode::MalformedKnownPayload &&
                              malformedUnit.diagnostic.message == "Unit successful result must be the exact empty object",
                          "filesystem Unit operations retain the exact-empty-object invariant");

        const auto wrongAssociation = detail::decodeClientOperationResult(detail::ClientRequestTarget::FsGetMetadata, file);
        result.expectTrue(!wrongAssociation && wrongAssociation.diagnostic.code == detail::ClientOperationDecodeCode::MalformedKnownPayload,
                          "a wrong filesystem result association fails through its target-specific decoder");
    }
} // namespace

int main() {
    tests::support::TestResult result;
    testOpenMatchTypesAndExperimentalBoundary(result);
    testFilesystemRequestEncoding(result);
    testFuzzyRequestEncoding(result);
    testFilesystemResponseDecoding(result);
    testFuzzyResponseDecoding(result);
    testNotificationDecoding(result);
    testOperationAssociations(result);
    return result.processResult();
}
