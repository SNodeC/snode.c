/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ApprovalCodec.h"

#include "ai/openai/codex/detail/DecodeDiagnostic.h"
#include "ai/openai/codex/typed/Conversation.h"
#include "ai/openai/codex/typed/Items.h"
#include "ai/openai/codex/typed/Types.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <nlohmann/detail/iterators/iter_impl.hpp>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
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

        bool decodeString(const Json& value, std::string& result) {
            if (!value.is_string()) {
                return false;
            }
            result = value.get_ref<const std::string&>();
            return true;
        }

        template <typename Strong>
        bool decodeStrongString(const Json& value, Strong& result) {
            return decodeString(value, result.value);
        }

        bool decodeBoolean(const Json& value, bool& result) {
            if (!value.is_boolean()) {
                return false;
            }
            result = value.get_ref<const Json::boolean_t&>();
            return true;
        }

        bool decodeInt64(const Json& value, std::int64_t& result) noexcept {
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

        bool decodePositiveUint64(const Json& value, std::uint64_t& result) noexcept {
            if (value.is_number_unsigned()) {
                result = value.get_ref<const Json::number_unsigned_t&>();
                return result >= 1;
            }
            if (!value.is_number_integer()) {
                return false;
            }
            const auto number = value.get_ref<const Json::number_integer_t&>();
            if (number < 1) {
                return false;
            }
            result = static_cast<std::uint64_t>(number);
            return true;
        }

        bool decodeStringArray(const Json& value, std::vector<std::string>& result) {
            if (!value.is_array()) {
                return false;
            }
            std::vector<std::string> decoded;
            decoded.reserve(value.size());
            for (const Json& element : value) {
                if (!element.is_string()) {
                    return false;
                }
                decoded.emplace_back(element.get_ref<const std::string&>());
            }
            result = std::move(decoded);
            return true;
        }

        template <typename T, typename Decode>
        bool decodeRequired(const Json& object, std::string_view name, T& result, Decode&& decode, std::string& invalidPath) {
            const Json* value = member(object, name);
            if (value == nullptr) {
                invalidPath = "$." + std::string(name);
                return false;
            }
            invalidPath.clear();
            if (!decode(*value, result)) {
                const std::string nestedPath = std::move(invalidPath);
                invalidPath = "$." + std::string(name);
                if (!nestedPath.empty() && nestedPath != "$" && nestedPath.starts_with("$")) {
                    invalidPath += nestedPath.substr(1);
                }
                return false;
            }
            return true;
        }

        template <typename T, typename Decode>
        bool decodeOptionalNullable(
            const Json& object, std::string_view name, typed::OptionalNullable<T>& result, Decode&& decode, std::string& invalidPath) {
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
            invalidPath.clear();
            if (!decode(*value, decoded)) {
                const std::string nestedPath = std::move(invalidPath);
                invalidPath = "$." + std::string(name);
                if (!nestedPath.empty() && nestedPath != "$" && nestedPath.starts_with("$")) {
                    invalidPath += nestedPath.substr(1);
                }
                return false;
            }
            result = typed::OptionalNullable<T>::withValue(std::move(decoded));
            return true;
        }

        template <typename T>
        bool validOptionalNullable(const typed::OptionalNullable<T>& value) noexcept {
            return value.present || !value.value.has_value();
        }

        template <typename T, typename Encode>
        bool encodeOptionalNullable(Json& object,
                                    std::string_view name,
                                    const typed::OptionalNullable<T>& value,
                                    Encode&& encode,
                                    std::string& error,
                                    std::string_view surface) {
            if (!validOptionalNullable(value)) {
                return fail(error, std::string(surface) + " field '$." + std::string(name) + "' has an inconsistent omitted state");
            }
            if (value.isOmitted()) {
                return true;
            }
            if (value.isNull()) {
                object[std::string(name)] = nullptr;
                return true;
            }
            std::optional<Json> encoded = encode(*value, error);
            if (!encoded) {
                return false;
            }
            object[std::string(name)] = std::move(*encoded);
            return true;
        }

        std::optional<Json> encodeString(const std::string& value, std::string&) {
            return std::optional<Json>{Json(value)};
        }

        std::optional<Json> encodeBoolean(const bool& value, std::string&) {
            return std::optional<Json>{Json(value)};
        }

        std::optional<Json> encodeStringArray(const std::vector<std::string>& value, std::string&) {
            return std::optional<Json>{Json(value)};
        }

        typed::DecodeDiagnostic prefixedDiagnostic(typed::DecodeDiagnostic diagnostic, std::string_view prefix) {
            if (diagnostic.fieldPath.empty() || diagnostic.fieldPath == "$") {
                diagnostic.fieldPath = std::string(prefix);
            } else if (diagnostic.fieldPath.starts_with("$")) {
                diagnostic.fieldPath = std::string(prefix) + diagnostic.fieldPath.substr(1);
            }
            return diagnostic;
        }

        void appendDiagnostic(std::vector<typed::DecodeDiagnostic>& target,
                              const std::optional<typed::DecodeDiagnostic>& diagnostic,
                              std::string_view prefix) {
            if (diagnostic) {
                target.emplace_back(prefixedDiagnostic(*diagnostic, prefix));
            }
        }

        void appendDiagnostics(std::vector<typed::DecodeDiagnostic>& target,
                               const std::vector<typed::DecodeDiagnostic>& source,
                               std::string_view prefix) {
            for (const typed::DecodeDiagnostic& diagnostic : source) {
                target.emplace_back(prefixedDiagnostic(diagnostic, prefix));
            }
        }

        std::optional<std::string> stringDiscriminator(const Json& value, std::string_view field) {
            const Json* discriminator = member(value, field);
            if (discriminator != nullptr && discriminator->is_string()) {
                return discriminator->get_ref<const std::string&>();
            }
            return std::nullopt;
        }

        template <typename Union, typename Alternative>
        ConversationDecodeResult<Union> decodedUnion(Alternative alternative,
                                                     std::optional<typed::DecodeDiagnostic> diagnostic = std::nullopt) {
            return {Union{std::move(alternative)}, std::move(diagnostic)};
        }

        template <typename Union, typename Unknown>
        ConversationDecodeResult<Union>
        preservedUnion(const Json& value, std::optional<std::string> discriminator, typed::DecodeDiagnostic diagnostic) {
            return {Union{Unknown{std::move(discriminator), value, diagnostic}}, std::move(diagnostic)};
        }

        template <typename Union, typename Unknown>
        ConversationDecodeResult<Union>
        malformedUnion(const Json& value, std::optional<std::string> discriminator, std::string_view surface, std::string path) {
            return preservedUnion<Union, Unknown>(
                value, std::move(discriminator), malformedKnownDiagnostic(std::string(surface), std::move(path)));
        }

        template <typename Union, typename Unknown>
        ConversationDecodeResult<Union>
        unknownUnion(const Json& value, std::optional<std::string> discriminator, std::string_view surface, std::string path) {
            return preservedUnion<Union, Unknown>(
                value, std::move(discriminator), unknownDiscriminatorDiagnostic(std::string(surface), std::move(path)));
        }

        bool decodeNetworkPolicyAmendment(const Json& value,
                                          typed::NetworkPolicyAmendment& result,
                                          std::optional<typed::DecodeDiagnostic>& diagnostic,
                                          std::string& invalidPath) {
            if (!value.is_object()) {
                invalidPath = "$";
                return false;
            }
            if (!decodeRequired(
                    value,
                    "action",
                    result.action,
                    [](const Json& nested, typed::NetworkPolicyRuleAction& decoded) {
                        return decodeString(nested, decoded.value);
                    },
                    invalidPath) ||
                !decodeRequired(value, "host", result.host, decodeString, invalidPath)) {
                return false;
            }
            if (!result.action.isKnown()) {
                diagnostic = unknownEnumDiagnostic("NetworkPolicyRuleAction", "$.action");
                result.diagnostics.push_back(*diagnostic);
            }
            result.raw = value;
            return true;
        }

        std::optional<Json> encodeNetworkPolicyAmendment(const typed::NetworkPolicyAmendment& value, std::string& error) {
            if (value.action.value.empty()) {
                fail(error, "NetworkPolicyAmendment field '$.action' must not be empty");
                return std::nullopt;
            }
            return std::optional<Json>{Json{{"action", value.action.value}, {"host", value.host}}};
        }

        std::optional<Json> encodeFileSystemSandboxEntry(const typed::FileSystemSandboxEntry& value, std::string& error);

        bool decodeFileSystemSandboxEntry(const Json& value,
                                          typed::FileSystemSandboxEntry& result,
                                          std::optional<typed::DecodeDiagnostic>& diagnostic,
                                          std::string& invalidPath);

    } // namespace

    ConversationDecodeResult<typed::FileSystemSpecialPath> decodeFileSystemSpecialPath(const Json& value) noexcept {
        constexpr std::string_view Surface = "FileSystemSpecialPath";
        try {
            if (!value.is_object()) {
                return malformedUnion<typed::FileSystemSpecialPath, typed::UnrecognizedFileSystemSpecialPath>(
                    value, std::nullopt, Surface, "$");
            }
            const std::optional<std::string> kind = stringDiscriminator(value, "kind");
            if (!kind) {
                return malformedUnion<typed::FileSystemSpecialPath, typed::UnrecognizedFileSystemSpecialPath>(
                    value, std::nullopt, Surface, "$.kind");
            }
            if (*kind == "root") {
                typed::RootFileSystemSpecialPath result;
                result.raw = value;
                return decodedUnion<typed::FileSystemSpecialPath>(std::move(result));
            }
            if (*kind == "minimal") {
                typed::MinimalFileSystemSpecialPath result;
                result.raw = value;
                return decodedUnion<typed::FileSystemSpecialPath>(std::move(result));
            }
            if (*kind == "tmpdir") {
                typed::TmpdirFileSystemSpecialPath result;
                result.raw = value;
                return decodedUnion<typed::FileSystemSpecialPath>(std::move(result));
            }
            if (*kind == "slash_tmp") {
                typed::SlashTmpFileSystemSpecialPath result;
                result.raw = value;
                return decodedUnion<typed::FileSystemSpecialPath>(std::move(result));
            }

            std::string invalidPath;
            if (*kind == "project_roots") {
                typed::ProjectRootsFileSystemSpecialPath result;
                if (!decodeOptionalNullable(value, "subpath", result.subpath, decodeString, invalidPath)) {
                    return malformedUnion<typed::FileSystemSpecialPath, typed::UnrecognizedFileSystemSpecialPath>(
                        value, kind, Surface, std::move(invalidPath));
                }
                result.raw = value;
                return decodedUnion<typed::FileSystemSpecialPath>(std::move(result));
            }
            if (*kind == "unknown") {
                typed::UnknownFileSystemSpecialPath result;
                if (!decodeRequired(value, "path", result.path, decodeString, invalidPath) ||
                    !decodeOptionalNullable(value, "subpath", result.subpath, decodeString, invalidPath)) {
                    return malformedUnion<typed::FileSystemSpecialPath, typed::UnrecognizedFileSystemSpecialPath>(
                        value, kind, Surface, std::move(invalidPath));
                }
                result.raw = value;
                return decodedUnion<typed::FileSystemSpecialPath>(std::move(result));
            }
            return unknownUnion<typed::FileSystemSpecialPath, typed::UnrecognizedFileSystemSpecialPath>(value, kind, Surface, "$.kind");
        } catch (...) {
            return malformedUnion<typed::FileSystemSpecialPath, typed::UnrecognizedFileSystemSpecialPath>(
                value, std::nullopt, Surface, "$");
        }
    }

    ConversationDecodeResult<typed::FileSystemPath> decodeFileSystemPath(const Json& value) noexcept {
        constexpr std::string_view Surface = "FileSystemPath";
        try {
            if (!value.is_object()) {
                return malformedUnion<typed::FileSystemPath, typed::UnrecognizedFileSystemPath>(value, std::nullopt, Surface, "$");
            }
            const std::optional<std::string> type = stringDiscriminator(value, "type");
            if (!type) {
                return malformedUnion<typed::FileSystemPath, typed::UnrecognizedFileSystemPath>(value, std::nullopt, Surface, "$.type");
            }
            std::string invalidPath;
            if (*type == "path") {
                typed::PathFileSystemPath result;
                if (!decodeRequired(value, "path", result.path, decodeString, invalidPath)) {
                    return malformedUnion<typed::FileSystemPath, typed::UnrecognizedFileSystemPath>(
                        value, type, Surface, std::move(invalidPath));
                }
                result.raw = value;
                return decodedUnion<typed::FileSystemPath>(std::move(result));
            }
            if (*type == "glob_pattern") {
                typed::GlobPatternFileSystemPath result;
                if (!decodeRequired(value, "pattern", result.pattern, decodeString, invalidPath)) {
                    return malformedUnion<typed::FileSystemPath, typed::UnrecognizedFileSystemPath>(
                        value, type, Surface, std::move(invalidPath));
                }
                result.raw = value;
                return decodedUnion<typed::FileSystemPath>(std::move(result));
            }
            if (*type == "special") {
                const Json* nested = member(value, "value");
                if (nested == nullptr) {
                    return malformedUnion<typed::FileSystemPath, typed::UnrecognizedFileSystemPath>(value, type, Surface, "$.value");
                }
                ConversationDecodeResult<typed::FileSystemSpecialPath> decoded = decodeFileSystemSpecialPath(*nested);
                if (!decoded.value) {
                    return malformedUnion<typed::FileSystemPath, typed::UnrecognizedFileSystemPath>(value, type, Surface, "$.value");
                }
                typed::SpecialFileSystemPath result;
                result.value = std::move(*decoded.value);
                appendDiagnostic(result.diagnostics, decoded.diagnostic, "$.value");
                result.raw = value;
                std::optional<typed::DecodeDiagnostic> diagnostic;
                if (decoded.diagnostic) {
                    diagnostic = prefixedDiagnostic(*decoded.diagnostic, "$.value");
                }
                return decodedUnion<typed::FileSystemPath>(std::move(result), std::move(diagnostic));
            }
            return unknownUnion<typed::FileSystemPath, typed::UnrecognizedFileSystemPath>(value, type, Surface, "$.type");
        } catch (...) {
            return malformedUnion<typed::FileSystemPath, typed::UnrecognizedFileSystemPath>(value, std::nullopt, Surface, "$");
        }
    }

    ConversationDecodeResult<typed::FileChange> decodeFileChange(const Json& value) noexcept {
        constexpr std::string_view Surface = "FileChange";
        try {
            if (!value.is_object()) {
                return malformedUnion<typed::FileChange, typed::UnrecognizedFileChange>(value, std::nullopt, Surface, "$");
            }
            const std::optional<std::string> type = stringDiscriminator(value, "type");
            if (!type) {
                return malformedUnion<typed::FileChange, typed::UnrecognizedFileChange>(value, std::nullopt, Surface, "$.type");
            }
            std::string invalidPath;
            if (*type == "add") {
                typed::AddFileChange result;
                if (!decodeRequired(value, "content", result.content, decodeString, invalidPath)) {
                    return malformedUnion<typed::FileChange, typed::UnrecognizedFileChange>(value, type, Surface, std::move(invalidPath));
                }
                result.raw = value;
                return decodedUnion<typed::FileChange>(std::move(result));
            }
            if (*type == "delete") {
                typed::DeleteFileChange result;
                if (!decodeRequired(value, "content", result.content, decodeString, invalidPath)) {
                    return malformedUnion<typed::FileChange, typed::UnrecognizedFileChange>(value, type, Surface, std::move(invalidPath));
                }
                result.raw = value;
                return decodedUnion<typed::FileChange>(std::move(result));
            }
            if (*type == "update") {
                typed::UpdateFileChange result;
                if (!decodeOptionalNullable(value, "move_path", result.movePath, decodeString, invalidPath) ||
                    !decodeRequired(value, "unified_diff", result.unifiedDiff, decodeString, invalidPath)) {
                    return malformedUnion<typed::FileChange, typed::UnrecognizedFileChange>(value, type, Surface, std::move(invalidPath));
                }
                result.raw = value;
                return decodedUnion<typed::FileChange>(std::move(result));
            }
            return unknownUnion<typed::FileChange, typed::UnrecognizedFileChange>(value, type, Surface, "$.type");
        } catch (...) {
            return malformedUnion<typed::FileChange, typed::UnrecognizedFileChange>(value, std::nullopt, Surface, "$");
        }
    }

    ConversationDecodeResult<typed::ParsedCommand> decodeParsedCommand(const Json& value) noexcept {
        constexpr std::string_view Surface = "ParsedCommand";
        try {
            if (!value.is_object()) {
                return malformedUnion<typed::ParsedCommand, typed::UnrecognizedParsedCommand>(value, std::nullopt, Surface, "$");
            }
            const std::optional<std::string> type = stringDiscriminator(value, "type");
            if (!type) {
                return malformedUnion<typed::ParsedCommand, typed::UnrecognizedParsedCommand>(value, std::nullopt, Surface, "$.type");
            }
            std::string invalidPath;
            std::string command;
            if (!decodeRequired(value, "cmd", command, decodeString, invalidPath)) {
                return malformedUnion<typed::ParsedCommand, typed::UnrecognizedParsedCommand>(value, type, Surface, std::move(invalidPath));
            }
            if (*type == "read") {
                typed::ReadParsedCommand result;
                result.command = std::move(command);
                if (!decodeRequired(value, "name", result.name, decodeString, invalidPath) ||
                    !decodeRequired(value, "path", result.path, decodeString, invalidPath)) {
                    return malformedUnion<typed::ParsedCommand, typed::UnrecognizedParsedCommand>(
                        value, type, Surface, std::move(invalidPath));
                }
                result.raw = value;
                return decodedUnion<typed::ParsedCommand>(std::move(result));
            }
            if (*type == "list_files") {
                typed::ListFilesParsedCommand result;
                result.command = std::move(command);
                if (!decodeOptionalNullable(value, "path", result.path, decodeString, invalidPath)) {
                    return malformedUnion<typed::ParsedCommand, typed::UnrecognizedParsedCommand>(
                        value, type, Surface, std::move(invalidPath));
                }
                result.raw = value;
                return decodedUnion<typed::ParsedCommand>(std::move(result));
            }
            if (*type == "search") {
                typed::SearchParsedCommand result;
                result.command = std::move(command);
                if (!decodeOptionalNullable(value, "path", result.path, decodeString, invalidPath) ||
                    !decodeOptionalNullable(value, "query", result.query, decodeString, invalidPath)) {
                    return malformedUnion<typed::ParsedCommand, typed::UnrecognizedParsedCommand>(
                        value, type, Surface, std::move(invalidPath));
                }
                result.raw = value;
                return decodedUnion<typed::ParsedCommand>(std::move(result));
            }
            if (*type == "unknown") {
                typed::UnknownParsedCommand result;
                result.command = std::move(command);
                result.raw = value;
                return decodedUnion<typed::ParsedCommand>(std::move(result));
            }
            return unknownUnion<typed::ParsedCommand, typed::UnrecognizedParsedCommand>(value, type, Surface, "$.type");
        } catch (...) {
            return malformedUnion<typed::ParsedCommand, typed::UnrecognizedParsedCommand>(value, std::nullopt, Surface, "$");
        }
    }

    ConversationDecodeResult<typed::CommandExecutionApprovalDecision> decodeCommandExecutionApprovalDecision(const Json& value) noexcept {
        constexpr std::string_view Surface = "CommandExecutionApprovalDecision";
        try {
            if (value.is_string()) {
                const std::string variant = value.get_ref<const std::string&>();
                if (variant == "accept") {
                    return decodedUnion<typed::CommandExecutionApprovalDecision>(typed::AcceptCommandExecutionApprovalDecision{});
                }
                if (variant == "acceptForSession") {
                    return decodedUnion<typed::CommandExecutionApprovalDecision>(typed::AcceptForSessionCommandExecutionApprovalDecision{});
                }
                if (variant == "decline") {
                    return decodedUnion<typed::CommandExecutionApprovalDecision>(typed::DeclineCommandExecutionApprovalDecision{});
                }
                if (variant == "cancel") {
                    return decodedUnion<typed::CommandExecutionApprovalDecision>(typed::CancelCommandExecutionApprovalDecision{});
                }
                if (variant == "acceptWithExecpolicyAmendment" || variant == "applyNetworkPolicyAmendment") {
                    return malformedUnion<typed::CommandExecutionApprovalDecision, typed::UnrecognizedCommandExecutionApprovalDecision>(
                        value, variant, Surface, "$");
                }
                return unknownUnion<typed::CommandExecutionApprovalDecision, typed::UnrecognizedCommandExecutionApprovalDecision>(
                    value, variant, Surface, "$");
            }
            if (!value.is_object() || value.empty()) {
                return malformedUnion<typed::CommandExecutionApprovalDecision, typed::UnrecognizedCommandExecutionApprovalDecision>(
                    value, std::nullopt, Surface, "$");
            }

            std::optional<std::string> variant;
            if (value.size() == 1) {
                variant = value.begin().key();
            } else {
                for (auto iterator = value.begin(); iterator != value.end(); ++iterator) {
                    if (iterator.key() == "acceptWithExecpolicyAmendment" || iterator.key() == "applyNetworkPolicyAmendment") {
                        variant = iterator.key();
                        break;
                    }
                }
                return malformedUnion<typed::CommandExecutionApprovalDecision, typed::UnrecognizedCommandExecutionApprovalDecision>(
                    value, std::move(variant), Surface, "$");
            }

            const auto nestedIterator = value.cbegin();
            const Json& nested = nestedIterator.value();
            if (*variant == "acceptWithExecpolicyAmendment") {
                typed::AcceptWithExecpolicyAmendmentCommandExecutionApprovalDecision result;
                std::string invalidPath;
                if (!nested.is_object() ||
                    !decodeRequired(nested, "execpolicy_amendment", result.execpolicyAmendment, decodeStringArray, invalidPath)) {
                    const std::string path =
                        nested.is_object() ? "$.acceptWithExecpolicyAmendment" + invalidPath.substr(1) : "$.acceptWithExecpolicyAmendment";
                    return malformedUnion<typed::CommandExecutionApprovalDecision, typed::UnrecognizedCommandExecutionApprovalDecision>(
                        value, variant, Surface, path);
                }
                return decodedUnion<typed::CommandExecutionApprovalDecision>(std::move(result));
            }
            if (*variant == "applyNetworkPolicyAmendment") {
                typed::ApplyNetworkPolicyAmendmentCommandExecutionApprovalDecision result;
                std::string invalidPath;
                std::optional<typed::DecodeDiagnostic> diagnostic;
                const Json* amendment = member(nested, "network_policy_amendment");
                if (!nested.is_object() || amendment == nullptr ||
                    !decodeNetworkPolicyAmendment(*amendment, result.networkPolicyAmendment, diagnostic, invalidPath)) {
                    std::string path = "$.applyNetworkPolicyAmendment";
                    if (nested.is_object()) {
                        path += ".network_policy_amendment";
                        if (amendment != nullptr && invalidPath != "$") {
                            path += invalidPath.substr(1);
                        }
                    }
                    return malformedUnion<typed::CommandExecutionApprovalDecision, typed::UnrecognizedCommandExecutionApprovalDecision>(
                        value, variant, Surface, std::move(path));
                }
                if (diagnostic) {
                    diagnostic = prefixedDiagnostic(*diagnostic, "$.applyNetworkPolicyAmendment.network_policy_amendment");
                }
                return decodedUnion<typed::CommandExecutionApprovalDecision>(std::move(result), std::move(diagnostic));
            }
            if (*variant == "accept" || *variant == "acceptForSession" || *variant == "decline" || *variant == "cancel") {
                return malformedUnion<typed::CommandExecutionApprovalDecision, typed::UnrecognizedCommandExecutionApprovalDecision>(
                    value, variant, Surface, "$");
            }
            return unknownUnion<typed::CommandExecutionApprovalDecision, typed::UnrecognizedCommandExecutionApprovalDecision>(
                value, variant, Surface, "$");
        } catch (...) {
            return malformedUnion<typed::CommandExecutionApprovalDecision, typed::UnrecognizedCommandExecutionApprovalDecision>(
                value, std::nullopt, Surface, "$");
        }
    }

    ConversationDecodeResult<typed::ReviewDecision> decodeReviewDecision(const Json& value) noexcept {
        constexpr std::string_view Surface = "ReviewDecision";
        try {
            if (value.is_string()) {
                const std::string variant = value.get_ref<const std::string&>();
                if (variant == "approved") {
                    return decodedUnion<typed::ReviewDecision>(typed::ApprovedReviewDecision{});
                }
                if (variant == "approved_for_session") {
                    return decodedUnion<typed::ReviewDecision>(typed::ApprovedForSessionReviewDecision{});
                }
                if (variant == "denied") {
                    return decodedUnion<typed::ReviewDecision>(typed::DeniedReviewDecision{});
                }
                if (variant == "timed_out") {
                    return decodedUnion<typed::ReviewDecision>(typed::TimedOutReviewDecision{});
                }
                if (variant == "abort") {
                    return decodedUnion<typed::ReviewDecision>(typed::AbortReviewDecision{});
                }
                if (variant == "approved_execpolicy_amendment" || variant == "network_policy_amendment") {
                    return malformedUnion<typed::ReviewDecision, typed::UnrecognizedReviewDecision>(value, variant, Surface, "$");
                }
                return unknownUnion<typed::ReviewDecision, typed::UnrecognizedReviewDecision>(value, variant, Surface, "$");
            }
            if (!value.is_object() || value.empty()) {
                return malformedUnion<typed::ReviewDecision, typed::UnrecognizedReviewDecision>(value, std::nullopt, Surface, "$");
            }

            std::optional<std::string> variant;
            if (value.size() == 1) {
                variant = value.begin().key();
            } else {
                for (auto iterator = value.begin(); iterator != value.end(); ++iterator) {
                    if (iterator.key() == "approved_execpolicy_amendment" || iterator.key() == "network_policy_amendment") {
                        variant = iterator.key();
                        break;
                    }
                }
                return malformedUnion<typed::ReviewDecision, typed::UnrecognizedReviewDecision>(value, std::move(variant), Surface, "$");
            }

            const auto nestedIterator = value.cbegin();
            const Json& nested = nestedIterator.value();
            if (*variant == "approved_execpolicy_amendment") {
                typed::ApprovedExecpolicyAmendmentReviewDecision result;
                std::string invalidPath;
                if (!nested.is_object() ||
                    !decodeRequired(
                        nested, "proposed_execpolicy_amendment", result.proposedExecpolicyAmendment, decodeStringArray, invalidPath)) {
                    const std::string path =
                        nested.is_object() ? "$.approved_execpolicy_amendment" + invalidPath.substr(1) : "$.approved_execpolicy_amendment";
                    return malformedUnion<typed::ReviewDecision, typed::UnrecognizedReviewDecision>(value, variant, Surface, path);
                }
                return decodedUnion<typed::ReviewDecision>(std::move(result));
            }
            if (*variant == "network_policy_amendment") {
                typed::NetworkPolicyAmendmentReviewDecision result;
                std::string invalidPath;
                std::optional<typed::DecodeDiagnostic> diagnostic;
                const Json* amendment = member(nested, "network_policy_amendment");
                if (!nested.is_object() || amendment == nullptr ||
                    !decodeNetworkPolicyAmendment(*amendment, result.networkPolicyAmendment, diagnostic, invalidPath)) {
                    std::string path = "$.network_policy_amendment";
                    if (nested.is_object()) {
                        path += ".network_policy_amendment";
                        if (amendment != nullptr && invalidPath != "$") {
                            path += invalidPath.substr(1);
                        }
                    }
                    return malformedUnion<typed::ReviewDecision, typed::UnrecognizedReviewDecision>(
                        value, variant, Surface, std::move(path));
                }
                if (diagnostic) {
                    diagnostic = prefixedDiagnostic(*diagnostic, "$.network_policy_amendment.network_policy_amendment");
                }
                return decodedUnion<typed::ReviewDecision>(std::move(result), std::move(diagnostic));
            }
            if (*variant == "approved" || *variant == "approved_for_session" || *variant == "denied" || *variant == "timed_out" ||
                *variant == "abort") {
                return malformedUnion<typed::ReviewDecision, typed::UnrecognizedReviewDecision>(value, variant, Surface, "$");
            }
            return unknownUnion<typed::ReviewDecision, typed::UnrecognizedReviewDecision>(value, variant, Surface, "$");
        } catch (...) {
            return malformedUnion<typed::ReviewDecision, typed::UnrecognizedReviewDecision>(value, std::nullopt, Surface, "$");
        }
    }

    std::optional<Json> encodeFileSystemSpecialPath(const typed::FileSystemSpecialPath& value, std::string& error) noexcept {
        try {
            error.clear();
            return std::visit(
                [&](const auto& alternative) -> std::optional<Json> {
                    using Alternative = std::decay_t<decltype(alternative)>;
                    if constexpr (std::is_same_v<Alternative, typed::RootFileSystemSpecialPath>) {
                        return std::optional<Json>{Json{{"kind", "root"}}};
                    } else if constexpr (std::is_same_v<Alternative, typed::MinimalFileSystemSpecialPath>) {
                        return std::optional<Json>{Json{{"kind", "minimal"}}};
                    } else if constexpr (std::is_same_v<Alternative, typed::TmpdirFileSystemSpecialPath>) {
                        return std::optional<Json>{Json{{"kind", "tmpdir"}}};
                    } else if constexpr (std::is_same_v<Alternative, typed::SlashTmpFileSystemSpecialPath>) {
                        return std::optional<Json>{Json{{"kind", "slash_tmp"}}};
                    } else if constexpr (std::is_same_v<Alternative, typed::ProjectRootsFileSystemSpecialPath>) {
                        Json result{{"kind", "project_roots"}};
                        if (!encodeOptionalNullable(result, "subpath", alternative.subpath, encodeString, error, "FileSystemSpecialPath")) {
                            return std::nullopt;
                        }
                        return std::optional<Json>{std::move(result)};
                    } else if constexpr (std::is_same_v<Alternative, typed::UnknownFileSystemSpecialPath>) {
                        Json result{{"kind", "unknown"}, {"path", alternative.path}};
                        if (!encodeOptionalNullable(result, "subpath", alternative.subpath, encodeString, error, "FileSystemSpecialPath")) {
                            return std::nullopt;
                        }
                        return std::optional<Json>{std::move(result)};
                    } else {
                        fail(error,
                             "unrecognized FileSystemSpecialPath alternative "
                             "cannot be encoded; use the raw protocol API");
                        return std::nullopt;
                    }
                },
                value);
        } catch (...) {
            fail(error, "FileSystemSpecialPath could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeFileSystemPath(const typed::FileSystemPath& value, std::string& error) noexcept {
        try {
            error.clear();
            return std::visit(
                [&](const auto& alternative) -> std::optional<Json> {
                    using Alternative = std::decay_t<decltype(alternative)>;
                    if constexpr (std::is_same_v<Alternative, typed::PathFileSystemPath>) {
                        return std::optional<Json>{Json{{"type", "path"}, {"path", alternative.path}}};
                    } else if constexpr (std::is_same_v<Alternative, typed::GlobPatternFileSystemPath>) {
                        return std::optional<Json>{Json{{"type", "glob_pattern"}, {"pattern", alternative.pattern}}};
                    } else if constexpr (std::is_same_v<Alternative, typed::SpecialFileSystemPath>) {
                        std::optional<Json> nested = encodeFileSystemSpecialPath(alternative.value, error);
                        if (!nested) {
                            return std::nullopt;
                        }
                        return std::optional<Json>{Json{{"type", "special"}, {"value", std::move(*nested)}}};
                    } else {
                        fail(error,
                             "unrecognized FileSystemPath alternative cannot be "
                             "encoded; use the raw protocol API");
                        return std::nullopt;
                    }
                },
                value);
        } catch (...) {
            fail(error, "FileSystemPath could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeFileChange(const typed::FileChange& value, std::string& error) noexcept {
        try {
            error.clear();
            return std::visit(
                [&](const auto& alternative) -> std::optional<Json> {
                    using Alternative = std::decay_t<decltype(alternative)>;
                    if constexpr (std::is_same_v<Alternative, typed::AddFileChange>) {
                        return std::optional<Json>{Json{{"type", "add"}, {"content", alternative.content}}};
                    } else if constexpr (std::is_same_v<Alternative, typed::DeleteFileChange>) {
                        return std::optional<Json>{Json{{"type", "delete"}, {"content", alternative.content}}};
                    } else if constexpr (std::is_same_v<Alternative, typed::UpdateFileChange>) {
                        Json result{{"type", "update"}, {"unified_diff", alternative.unifiedDiff}};
                        if (!encodeOptionalNullable(result, "move_path", alternative.movePath, encodeString, error, "FileChange")) {
                            return std::nullopt;
                        }
                        return std::optional<Json>{std::move(result)};
                    } else {
                        fail(error,
                             "unrecognized FileChange alternative cannot be "
                             "encoded; use the raw protocol API");
                        return std::nullopt;
                    }
                },
                value);
        } catch (...) {
            fail(error, "FileChange could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeParsedCommand(const typed::ParsedCommand& value, std::string& error) noexcept {
        try {
            error.clear();
            return std::visit(
                [&](const auto& alternative) -> std::optional<Json> {
                    using Alternative = std::decay_t<decltype(alternative)>;
                    if constexpr (std::is_same_v<Alternative, typed::ReadParsedCommand>) {
                        return std::optional<Json>{
                            Json{{"type", "read"}, {"cmd", alternative.command}, {"name", alternative.name}, {"path", alternative.path}}};
                    } else if constexpr (std::is_same_v<Alternative, typed::ListFilesParsedCommand>) {
                        Json result{{"type", "list_files"}, {"cmd", alternative.command}};
                        if (!encodeOptionalNullable(result, "path", alternative.path, encodeString, error, "ParsedCommand")) {
                            return std::nullopt;
                        }
                        return std::optional<Json>{std::move(result)};
                    } else if constexpr (std::is_same_v<Alternative, typed::SearchParsedCommand>) {
                        Json result{{"type", "search"}, {"cmd", alternative.command}};
                        if (!encodeOptionalNullable(result, "path", alternative.path, encodeString, error, "ParsedCommand") ||
                            !encodeOptionalNullable(result, "query", alternative.query, encodeString, error, "ParsedCommand")) {
                            return std::nullopt;
                        }
                        return std::optional<Json>{std::move(result)};
                    } else if constexpr (std::is_same_v<Alternative, typed::UnknownParsedCommand>) {
                        return std::optional<Json>{Json{{"type", "unknown"}, {"cmd", alternative.command}}};
                    } else {
                        fail(error,
                             "unrecognized ParsedCommand alternative cannot be "
                             "encoded; use the raw protocol API");
                        return std::nullopt;
                    }
                },
                value);
        } catch (...) {
            fail(error, "ParsedCommand could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeCommandExecutionApprovalDecision(const typed::CommandExecutionApprovalDecision& value,
                                                               std::string& error) noexcept {
        try {
            error.clear();
            return std::visit(
                [&](const auto& alternative) -> std::optional<Json> {
                    using Alternative = std::decay_t<decltype(alternative)>;
                    if constexpr (std::is_same_v<Alternative, typed::AcceptCommandExecutionApprovalDecision>) {
                        return std::optional<Json>{Json("accept")};
                    } else if constexpr (std::is_same_v<Alternative, typed::AcceptForSessionCommandExecutionApprovalDecision>) {
                        return std::optional<Json>{Json("acceptForSession")};
                    } else if constexpr (std::is_same_v<Alternative,
                                                        typed::AcceptWithExecpolicyAmendmentCommandExecutionApprovalDecision>) {
                        return std::optional<Json>{
                            Json{{"acceptWithExecpolicyAmendment", {{"execpolicy_amendment", alternative.execpolicyAmendment}}}}};
                    } else if constexpr (std::is_same_v<Alternative, typed::ApplyNetworkPolicyAmendmentCommandExecutionApprovalDecision>) {
                        std::optional<Json> amendment = encodeNetworkPolicyAmendment(alternative.networkPolicyAmendment, error);
                        if (!amendment) {
                            return std::nullopt;
                        }
                        return std::optional<Json>{
                            Json{{"applyNetworkPolicyAmendment", {{"network_policy_amendment", std::move(*amendment)}}}}};
                    } else if constexpr (std::is_same_v<Alternative, typed::DeclineCommandExecutionApprovalDecision>) {
                        return std::optional<Json>{Json("decline")};
                    } else if constexpr (std::is_same_v<Alternative, typed::CancelCommandExecutionApprovalDecision>) {
                        return std::optional<Json>{Json("cancel")};
                    } else {
                        fail(error,
                             "unrecognized CommandExecutionApprovalDecision "
                             "alternative cannot be encoded; use the raw "
                             "protocol API");
                        return std::nullopt;
                    }
                },
                value);
        } catch (...) {
            fail(error, "CommandExecutionApprovalDecision could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeReviewDecision(const typed::ReviewDecision& value, std::string& error) noexcept {
        try {
            error.clear();
            return std::visit(
                [&](const auto& alternative) -> std::optional<Json> {
                    using Alternative = std::decay_t<decltype(alternative)>;
                    if constexpr (std::is_same_v<Alternative, typed::ApprovedReviewDecision>) {
                        return std::optional<Json>{Json("approved")};
                    } else if constexpr (std::is_same_v<Alternative, typed::ApprovedExecpolicyAmendmentReviewDecision>) {
                        return std::optional<Json>{Json{{"approved_execpolicy_amendment",
                                                         {{"proposed_execpolicy_amendment", alternative.proposedExecpolicyAmendment}}}}};
                    } else if constexpr (std::is_same_v<Alternative, typed::ApprovedForSessionReviewDecision>) {
                        return std::optional<Json>{Json("approved_for_session")};
                    } else if constexpr (std::is_same_v<Alternative, typed::NetworkPolicyAmendmentReviewDecision>) {
                        std::optional<Json> amendment = encodeNetworkPolicyAmendment(alternative.networkPolicyAmendment, error);
                        if (!amendment) {
                            return std::nullopt;
                        }
                        return std::optional<Json>{
                            Json{{"network_policy_amendment", {{"network_policy_amendment", std::move(*amendment)}}}}};
                    } else if constexpr (std::is_same_v<Alternative, typed::DeniedReviewDecision>) {
                        return std::optional<Json>{Json("denied")};
                    } else if constexpr (std::is_same_v<Alternative, typed::TimedOutReviewDecision>) {
                        return std::optional<Json>{Json("timed_out")};
                    } else if constexpr (std::is_same_v<Alternative, typed::AbortReviewDecision>) {
                        return std::optional<Json>{Json("abort")};
                    } else {
                        fail(error,
                             "unrecognized ReviewDecision alternative cannot be "
                             "encoded; use the raw protocol API");
                        return std::nullopt;
                    }
                },
                value);
        } catch (...) {
            fail(error, "ReviewDecision could not be encoded");
            return std::nullopt;
        }
    }

    namespace {

        bool decodeFileSystemSandboxEntry(const Json& value,
                                          typed::FileSystemSandboxEntry& result,
                                          std::optional<typed::DecodeDiagnostic>& diagnostic,
                                          std::string& invalidPath) {
            if (!value.is_object()) {
                invalidPath = "$";
                return false;
            }
            if (!decodeRequired(
                    value,
                    "access",
                    result.access,
                    [](const Json& nested, typed::FileSystemAccessMode& decoded) {
                        return decodeString(nested, decoded.value);
                    },
                    invalidPath)) {
                return false;
            }
            const Json* path = member(value, "path");
            if (path == nullptr) {
                invalidPath = "$.path";
                return false;
            }
            ConversationDecodeResult<typed::FileSystemPath> decodedPath = decodeFileSystemPath(*path);
            if (!decodedPath.value) {
                invalidPath = "$.path";
                return false;
            }
            result.path = std::move(*decodedPath.value);
            if (!result.access.isKnown()) {
                result.diagnostics.emplace_back(unknownEnumDiagnostic("FileSystemAccessMode", "$.access"));
                diagnostic = result.diagnostics.back();
            }
            if (decodedPath.diagnostic) {
                result.diagnostics.emplace_back(prefixedDiagnostic(*decodedPath.diagnostic, "$.path"));
                if (!diagnostic) {
                    diagnostic = result.diagnostics.back();
                }
            }
            result.raw = value;
            return true;
        }

        std::optional<Json> encodeFileSystemSandboxEntry(const typed::FileSystemSandboxEntry& value, std::string& error) {
            if (value.access.value.empty()) {
                fail(error, "FileSystemSandboxEntry field '$.access' must not be empty");
                return std::nullopt;
            }
            std::optional<Json> path = encodeFileSystemPath(value.path, error);
            if (!path) {
                return std::nullopt;
            }
            return std::optional<Json>{Json{{"access", value.access.value}, {"path", std::move(*path)}}};
        }

        bool decodeAdditionalFileSystemPermissions(const Json& value,
                                                   typed::AdditionalFileSystemPermissions& result,
                                                   std::string& error,
                                                   std::string_view surface) {
            if (!value.is_object()) {
                return fail(error, std::string(surface) + " at '$' must be an object");
            }
            std::string invalidPath;
            if (!decodeOptionalNullable(
                    value,
                    "entries",
                    result.entries,
                    [&](const Json& nested, std::vector<typed::FileSystemSandboxEntry>& decoded) {
                        if (!nested.is_array()) {
                            return false;
                        }
                        decoded.reserve(nested.size());
                        std::size_t index = 0;
                        for (const Json& rawEntry : nested) {
                            typed::FileSystemSandboxEntry entry;
                            std::optional<typed::DecodeDiagnostic> diagnostic;
                            std::string entryPath;
                            if (!decodeFileSystemSandboxEntry(rawEntry, entry, diagnostic, entryPath)) {
                                invalidPath = "$[" + std::to_string(index) + "]" + (entryPath == "$" ? "" : entryPath.substr(1));
                                return false;
                            }
                            appendDiagnostics(result.diagnostics, entry.diagnostics, "$.entries[" + std::to_string(index) + "]");
                            decoded.emplace_back(std::move(entry));
                            ++index;
                        }
                        return true;
                    },
                    invalidPath) ||
                !decodeOptionalNullable(value, "globScanMaxDepth", result.globScanMaxDepth, decodePositiveUint64, invalidPath) ||
                !decodeOptionalNullable(value, "read", result.read, decodeStringArray, invalidPath) ||
                !decodeOptionalNullable(value, "write", result.write, decodeStringArray, invalidPath)) {
                if (invalidPath.empty()) {
                    invalidPath = "$";
                }
                return fail(error,
                            std::string(surface) + " field '" + invalidPath +
                                "' has the wrong type or violates its integer "
                                "bounds");
            }
            result.raw = value;
            return true;
        }

        bool decodeAdditionalNetworkPermissions(const Json& value,
                                                typed::AdditionalNetworkPermissions& result,
                                                std::string& error,
                                                std::string_view surface) {
            if (!value.is_object()) {
                return fail(error, std::string(surface) + " at '$' must be an object");
            }
            std::string invalidPath;
            if (!decodeOptionalNullable(value, "enabled", result.enabled, decodeBoolean, invalidPath)) {
                return fail(error, std::string(surface) + " field '" + invalidPath + "' has the wrong type");
            }
            result.raw = value;
            return true;
        }

        template <typename Profile>
        std::optional<Profile>
        decodePermissionProfile(const Json& value, std::string& error, std::string_view surface, bool rejectExtraFields) {
            if (!value.is_object()) {
                fail(error, std::string(surface) + " at '$' must be an object");
                return std::nullopt;
            }
            if (rejectExtraFields) {
                for (auto iterator = value.begin(); iterator != value.end(); ++iterator) {
                    if (iterator.key() != "fileSystem" && iterator.key() != "network") {
                        fail(error, std::string(surface) + " field '$." + iterator.key() + "' is not permitted by the stable schema");
                        return std::nullopt;
                    }
                }
            }

            Profile result;
            std::string invalidPath;
            if (!decodeOptionalNullable(
                    value,
                    "fileSystem",
                    result.fileSystem,
                    [&](const Json& nested, typed::AdditionalFileSystemPermissions& decoded) {
                        std::string nestedError;
                        if (!decodeAdditionalFileSystemPermissions(nested, decoded, nestedError, "AdditionalFileSystemPermissions")) {
                            return false;
                        }
                        appendDiagnostics(result.diagnostics, decoded.diagnostics, "$.fileSystem");
                        return true;
                    },
                    invalidPath) ||
                !decodeOptionalNullable(
                    value,
                    "network",
                    result.network,
                    [&](const Json& nested, typed::AdditionalNetworkPermissions& decoded) {
                        std::string nestedError;
                        return decodeAdditionalNetworkPermissions(nested, decoded, nestedError, "AdditionalNetworkPermissions");
                    },
                    invalidPath)) {
                if (invalidPath.empty()) {
                    invalidPath = "$";
                }
                fail(error, std::string(surface) + " field '" + invalidPath + "' has the wrong type or malformed nested value");
                return std::nullopt;
            }
            result.raw = value;
            error.clear();
            return std::optional<Profile>{std::move(result)};
        }

        std::optional<Json> encodeAdditionalFileSystemPermissions(const typed::AdditionalFileSystemPermissions& value, std::string& error) {
            Json result = Json::object();
            if (!encodeOptionalNullable(
                    result,
                    "entries",
                    value.entries,
                    [&](const std::vector<typed::FileSystemSandboxEntry>& entries, std::string& nestedError) -> std::optional<Json> {
                        Json encoded = Json::array();
                        for (const typed::FileSystemSandboxEntry& entry : entries) {
                            std::optional<Json> item = encodeFileSystemSandboxEntry(entry, nestedError);
                            if (!item) {
                                return std::nullopt;
                            }
                            encoded.push_back(std::move(*item));
                        }
                        return std::optional<Json>{std::move(encoded)};
                    },
                    error,
                    "AdditionalFileSystemPermissions") ||
                !encodeOptionalNullable(
                    result,
                    "globScanMaxDepth",
                    value.globScanMaxDepth,
                    [](const std::uint64_t& depth, std::string& nestedError) -> std::optional<Json> {
                        if (depth < 1) {
                            fail(nestedError,
                                 "AdditionalFileSystemPermissions field "
                                 "'$.globScanMaxDepth' must be at least 1");
                            return std::nullopt;
                        }
                        return std::optional<Json>{Json(depth)};
                    },
                    error,
                    "AdditionalFileSystemPermissions") ||
                !encodeOptionalNullable(result, "read", value.read, encodeStringArray, error, "AdditionalFileSystemPermissions") ||
                !encodeOptionalNullable(result, "write", value.write, encodeStringArray, error, "AdditionalFileSystemPermissions")) {
                return std::nullopt;
            }
            return std::optional<Json>{std::move(result)};
        }

        std::optional<Json> encodeAdditionalNetworkPermissions(const typed::AdditionalNetworkPermissions& value, std::string& error) {
            Json result = Json::object();
            if (!encodeOptionalNullable(result, "enabled", value.enabled, encodeBoolean, error, "AdditionalNetworkPermissions")) {
                return std::nullopt;
            }
            return std::optional<Json>{std::move(result)};
        }

        template <typename Profile>
        std::optional<Json> encodePermissionProfile(const Profile& value, std::string& error, std::string_view surface) {
            Json result = Json::object();
            if (!encodeOptionalNullable(result, "fileSystem", value.fileSystem, encodeAdditionalFileSystemPermissions, error, surface) ||
                !encodeOptionalNullable(result, "network", value.network, encodeAdditionalNetworkPermissions, error, surface)) {
                return std::nullopt;
            }
            return std::optional<Json>{std::move(result)};
        }

    } // namespace

    std::optional<typed::RequestPermissionProfile> decodeRequestPermissionProfile(const Json& value, std::string& error) noexcept {
        try {
            return decodePermissionProfile<typed::RequestPermissionProfile>(value, error, "RequestPermissionProfile", true);
        } catch (...) {
            fail(error, "RequestPermissionProfile could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::GrantedPermissionProfile> decodeGrantedPermissionProfile(const Json& value, std::string& error) noexcept {
        try {
            return decodePermissionProfile<typed::GrantedPermissionProfile>(value, error, "GrantedPermissionProfile", false);
        } catch (...) {
            fail(error, "GrantedPermissionProfile could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeRequestPermissionProfile(const typed::RequestPermissionProfile& value, std::string& error) noexcept {
        try {
            return encodePermissionProfile(value, error, "RequestPermissionProfile");
        } catch (...) {
            fail(error, "RequestPermissionProfile could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeGrantedPermissionProfile(const typed::GrantedPermissionProfile& value, std::string& error) noexcept {
        try {
            return encodePermissionProfile(value, error, "GrantedPermissionProfile");
        } catch (...) {
            fail(error, "GrantedPermissionProfile could not be encoded");
            return std::nullopt;
        }
    }

    namespace {

        bool decodeNetworkApprovalContext(const Json& value,
                                          typed::NetworkApprovalContext& result,
                                          std::optional<typed::DecodeDiagnostic>& diagnostic,
                                          std::string& invalidPath) {
            if (!value.is_object()) {
                invalidPath = "$";
                return false;
            }
            if (!decodeRequired(value, "host", result.host, decodeString, invalidPath) ||
                !decodeRequired(
                    value,
                    "protocol",
                    result.protocol,
                    [](const Json& nested, typed::NetworkApprovalProtocol& decoded) {
                        return decodeString(nested, decoded.value);
                    },
                    invalidPath)) {
                return false;
            }
            if (!result.protocol.isKnown()) {
                diagnostic = unknownEnumDiagnostic("NetworkApprovalProtocol", "$.protocol");
                result.diagnostics.push_back(*diagnostic);
            }
            result.raw = value;
            return true;
        }

        bool decodeNetworkPolicyAmendmentArray(const Json& value,
                                               std::vector<typed::NetworkPolicyAmendment>& result,
                                               std::vector<typed::DecodeDiagnostic>& diagnostics,
                                               std::string& invalidPath) {
            if (!value.is_array()) {
                return false;
            }
            result.reserve(value.size());
            std::size_t index = 0;
            for (const Json& rawAmendment : value) {
                typed::NetworkPolicyAmendment amendment;
                std::optional<typed::DecodeDiagnostic> diagnostic;
                std::string nestedPath;
                if (!decodeNetworkPolicyAmendment(rawAmendment, amendment, diagnostic, nestedPath)) {
                    invalidPath = "$[" + std::to_string(index) + "]" + (nestedPath == "$" ? "" : nestedPath.substr(1));
                    return false;
                }
                appendDiagnostics(diagnostics, amendment.diagnostics, "$[" + std::to_string(index) + "]");
                result.emplace_back(std::move(amendment));
                ++index;
            }
            return true;
        }

        bool decodeCommandActionArray(const Json& value,
                                      std::vector<typed::CommandAction>& result,
                                      std::vector<typed::DecodeDiagnostic>& diagnostics) {
            if (!value.is_array()) {
                return false;
            }
            result.reserve(value.size());
            std::size_t index = 0;
            for (const Json& rawAction : value) {
                ConversationDecodeResult<typed::CommandAction> decoded = decodeCommandAction(rawAction);
                if (!decoded.value) {
                    typed::DecodeDiagnostic diagnostic = decoded.diagnostic.value_or(malformedKnownDiagnostic("CommandAction", "$"));
                    std::optional<std::string> type = stringDiscriminator(rawAction, "type");
                    result.emplace_back(typed::UnrecognizedCommandAction{std::move(type), rawAction, diagnostic});
                    diagnostics.emplace_back(prefixedDiagnostic(diagnostic, "$[" + std::to_string(index) + "]"));
                } else {
                    result.emplace_back(std::move(*decoded.value));
                    appendDiagnostic(diagnostics, decoded.diagnostic, "$[" + std::to_string(index) + "]");
                }
                ++index;
            }
            return true;
        }

        bool decodeParsedCommandArray(const Json& value,
                                      std::vector<typed::ParsedCommand>& result,
                                      std::vector<typed::DecodeDiagnostic>& diagnostics) {
            if (!value.is_array()) {
                return false;
            }
            result.reserve(value.size());
            std::size_t index = 0;
            for (const Json& rawCommand : value) {
                ConversationDecodeResult<typed::ParsedCommand> decoded = decodeParsedCommand(rawCommand);
                if (!decoded.value) {
                    return false;
                }
                result.emplace_back(std::move(*decoded.value));
                appendDiagnostic(diagnostics, decoded.diagnostic, "$[" + std::to_string(index) + "]");
                ++index;
            }
            return true;
        }

        bool decodeFileChangeMap(const Json& value,
                                 std::map<std::string, typed::FileChange>& result,
                                 std::vector<typed::DecodeDiagnostic>& diagnostics) {
            if (!value.is_object()) {
                return false;
            }
            for (auto iterator = value.begin(); iterator != value.end(); ++iterator) {
                ConversationDecodeResult<typed::FileChange> decoded = decodeFileChange(iterator.value());
                if (!decoded.value) {
                    return false;
                }
                // Map keys are filesystem paths and therefore sensitive. Keep
                // the structural location useful without copying a path into
                // diagnostics.
                appendDiagnostic(diagnostics, decoded.diagnostic, "$.*");
                result.emplace(iterator.key(), std::move(*decoded.value));
            }
            return true;
        }

        template <typename T>
        bool requireObject(const Json& value, std::string_view surface, std::optional<T>& result, std::string& error) {
            if (value.is_object()) {
                return true;
            }
            fail(error, std::string(surface) + " at '$' must be an object");
            result.reset();
            return false;
        }

    } // namespace

    std::optional<typed::ApplyPatchApprovalParams> decodeApplyPatchApprovalParams(const Json& value, std::string& error) noexcept {
        try {
            std::optional<typed::ApplyPatchApprovalParams> result{std::in_place};
            if (!requireObject(value, "ApplyPatchApprovalParams", result, error)) {
                return std::nullopt;
            }
            std::string invalidPath;
            if (!decodeRequired(value, "callId", result->callId, decodeStrongString<typed::ResponseCallId>, invalidPath) ||
                !decodeRequired(value, "conversationId", result->conversationId, decodeStrongString<typed::ThreadId>, invalidPath) ||
                !decodeRequired(
                    value,
                    "fileChanges",
                    result->fileChanges,
                    [&](const Json& nested, std::map<std::string, typed::FileChange>& decoded) {
                        std::vector<typed::DecodeDiagnostic> nestedDiagnostics;
                        const bool success = decodeFileChangeMap(nested, decoded, nestedDiagnostics);
                        if (success) {
                            appendDiagnostics(result->diagnostics, nestedDiagnostics, "$.fileChanges");
                        }
                        return success;
                    },
                    invalidPath) ||
                !decodeOptionalNullable(value, "grantRoot", result->grantRoot, decodeString, invalidPath) ||
                !decodeOptionalNullable(value, "reason", result->reason, decodeString, invalidPath)) {
                fail(error, "ApplyPatchApprovalParams field '" + invalidPath + "' has the wrong type or is missing");
                return std::nullopt;
            }
            result->raw = value;
            error.clear();
            return std::move(*result);
        } catch (...) {
            fail(error, "ApplyPatchApprovalParams could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::ExecCommandApprovalParams> decodeExecCommandApprovalParams(const Json& value, std::string& error) noexcept {
        try {
            std::optional<typed::ExecCommandApprovalParams> result{std::in_place};
            if (!requireObject(value, "ExecCommandApprovalParams", result, error)) {
                return std::nullopt;
            }
            std::string invalidPath;
            if (!decodeOptionalNullable(value, "approvalId", result->approvalId, decodeString, invalidPath) ||
                !decodeRequired(value, "callId", result->callId, decodeStrongString<typed::ResponseCallId>, invalidPath) ||
                !decodeRequired(value, "command", result->command, decodeStringArray, invalidPath) ||
                !decodeRequired(value, "conversationId", result->conversationId, decodeStrongString<typed::ThreadId>, invalidPath) ||
                !decodeRequired(value, "cwd", result->cwd, decodeString, invalidPath) ||
                !decodeRequired(
                    value,
                    "parsedCmd",
                    result->parsedCommand,
                    [&](const Json& nested, std::vector<typed::ParsedCommand>& decoded) {
                        std::vector<typed::DecodeDiagnostic> nestedDiagnostics;
                        const bool success = decodeParsedCommandArray(nested, decoded, nestedDiagnostics);
                        if (success) {
                            appendDiagnostics(result->diagnostics, nestedDiagnostics, "$.parsedCmd");
                        }
                        return success;
                    },
                    invalidPath) ||
                !decodeOptionalNullable(value, "reason", result->reason, decodeString, invalidPath)) {
                fail(error, "ExecCommandApprovalParams field '" + invalidPath + "' has the wrong type or is missing");
                return std::nullopt;
            }
            result->raw = value;
            error.clear();
            return std::move(*result);
        } catch (...) {
            fail(error, "ExecCommandApprovalParams could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::CommandExecutionRequestApprovalParams> decodeCommandExecutionRequestApprovalParams(const Json& value,
                                                                                                            std::string& error) noexcept {
        try {
            std::optional<typed::CommandExecutionRequestApprovalParams> result{std::in_place};
            if (!requireObject(value, "CommandExecutionRequestApprovalParams", result, error)) {
                return std::nullopt;
            }
            std::string invalidPath;
            if (!decodeOptionalNullable(value, "approvalId", result->approvalId, decodeString, invalidPath) ||
                !decodeOptionalNullable(value, "command", result->command, decodeString, invalidPath) ||
                !decodeOptionalNullable(
                    value,
                    "commandActions",
                    result->commandActions,
                    [&](const Json& nested, std::vector<typed::CommandAction>& decoded) {
                        std::vector<typed::DecodeDiagnostic> nestedDiagnostics;
                        const bool success = decodeCommandActionArray(nested, decoded, nestedDiagnostics);
                        if (success) {
                            appendDiagnostics(result->diagnostics, nestedDiagnostics, "$.commandActions");
                        }
                        return success;
                    },
                    invalidPath) ||
                !decodeOptionalNullable(value, "cwd", result->cwd, decodeStrongString<typed::LegacyAppPathString>, invalidPath) ||
                !decodeOptionalNullable(value, "environmentId", result->environmentId, decodeString, invalidPath) ||
                !decodeRequired(value, "itemId", result->itemId, decodeStrongString<typed::ItemId>, invalidPath) ||
                !decodeOptionalNullable(
                    value,
                    "networkApprovalContext",
                    result->networkApprovalContext,
                    [&](const Json& nested, typed::NetworkApprovalContext& decoded) {
                        std::optional<typed::DecodeDiagnostic> diagnostic;
                        std::string nestedPath;
                        const bool success = decodeNetworkApprovalContext(nested, decoded, diagnostic, nestedPath);
                        if (success) {
                            appendDiagnostics(result->diagnostics, decoded.diagnostics, "$.networkApprovalContext");
                        } else {
                            invalidPath = std::move(nestedPath);
                        }
                        return success;
                    },
                    invalidPath) ||
                !decodeOptionalNullable(
                    value, "proposedExecpolicyAmendment", result->proposedExecpolicyAmendment, decodeStringArray, invalidPath) ||
                !decodeOptionalNullable(
                    value,
                    "proposedNetworkPolicyAmendments",
                    result->proposedNetworkPolicyAmendments,
                    [&](const Json& nested, std::vector<typed::NetworkPolicyAmendment>& decoded) {
                        std::string nestedPath;
                        std::vector<typed::DecodeDiagnostic> nestedDiagnostics;
                        const bool success = decodeNetworkPolicyAmendmentArray(nested, decoded, nestedDiagnostics, nestedPath);
                        if (success) {
                            appendDiagnostics(result->diagnostics, nestedDiagnostics, "$.proposedNetworkPolicyAmendments");
                        } else {
                            invalidPath = std::move(nestedPath);
                        }
                        return success;
                    },
                    invalidPath) ||
                !decodeOptionalNullable(value, "reason", result->reason, decodeString, invalidPath) ||
                !decodeRequired(value, "startedAtMs", result->startedAtMs, decodeInt64, invalidPath) ||
                !decodeRequired(value, "threadId", result->threadId, decodeStrongString<typed::ThreadId>, invalidPath) ||
                !decodeRequired(value, "turnId", result->turnId, decodeStrongString<typed::TurnId>, invalidPath)) {
                fail(error, "CommandExecutionRequestApprovalParams field '" + invalidPath + "' has the wrong type or is missing");
                return std::nullopt;
            }
            result->raw = value;
            error.clear();
            return std::move(*result);
        } catch (...) {
            fail(error, "CommandExecutionRequestApprovalParams could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::FileChangeRequestApprovalParams> decodeFileChangeRequestApprovalParams(const Json& value,
                                                                                                std::string& error) noexcept {
        try {
            std::optional<typed::FileChangeRequestApprovalParams> result{std::in_place};
            if (!requireObject(value, "FileChangeRequestApprovalParams", result, error)) {
                return std::nullopt;
            }
            std::string invalidPath;
            if (!decodeOptionalNullable(value, "grantRoot", result->grantRoot, decodeString, invalidPath) ||
                !decodeRequired(value, "itemId", result->itemId, decodeStrongString<typed::ItemId>, invalidPath) ||
                !decodeOptionalNullable(value, "reason", result->reason, decodeString, invalidPath) ||
                !decodeRequired(value, "startedAtMs", result->startedAtMs, decodeInt64, invalidPath) ||
                !decodeRequired(value, "threadId", result->threadId, decodeStrongString<typed::ThreadId>, invalidPath) ||
                !decodeRequired(value, "turnId", result->turnId, decodeStrongString<typed::TurnId>, invalidPath)) {
                fail(error, "FileChangeRequestApprovalParams field '" + invalidPath + "' has the wrong type or is missing");
                return std::nullopt;
            }
            result->raw = value;
            error.clear();
            return std::move(*result);
        } catch (...) {
            fail(error, "FileChangeRequestApprovalParams could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::PermissionsRequestApprovalParams> decodePermissionsRequestApprovalParams(const Json& value,
                                                                                                  std::string& error) noexcept {
        try {
            std::optional<typed::PermissionsRequestApprovalParams> result{std::in_place};
            if (!requireObject(value, "PermissionsRequestApprovalParams", result, error)) {
                return std::nullopt;
            }
            std::string invalidPath;
            if (!decodeRequired(value, "cwd", result->cwd, decodeStrongString<typed::AbsolutePathBuf>, invalidPath) ||
                !decodeOptionalNullable(value, "environmentId", result->environmentId, decodeString, invalidPath) ||
                !decodeRequired(value, "itemId", result->itemId, decodeStrongString<typed::ItemId>, invalidPath)) {
                fail(error, "PermissionsRequestApprovalParams field '" + invalidPath + "' has the wrong type or is missing");
                return std::nullopt;
            }
            const Json* permissions = member(value, "permissions");
            if (permissions == nullptr) {
                fail(error,
                     "PermissionsRequestApprovalParams field '$.permissions' "
                     "is missing");
                return std::nullopt;
            }
            std::string permissionError;
            std::optional<typed::RequestPermissionProfile> decodedPermissions =
                decodeRequestPermissionProfile(*permissions, permissionError);
            if (!decodedPermissions) {
                fail(error,
                     "PermissionsRequestApprovalParams field '$.permissions' "
                     "is malformed");
                return std::nullopt;
            }
            result->permissions = std::move(*decodedPermissions);
            appendDiagnostics(result->diagnostics, result->permissions.diagnostics, "$.permissions");
            if (!decodeOptionalNullable(value, "reason", result->reason, decodeString, invalidPath) ||
                !decodeRequired(value, "startedAtMs", result->startedAtMs, decodeInt64, invalidPath) ||
                !decodeRequired(value, "threadId", result->threadId, decodeStrongString<typed::ThreadId>, invalidPath) ||
                !decodeRequired(value, "turnId", result->turnId, decodeStrongString<typed::TurnId>, invalidPath)) {
                fail(error, "PermissionsRequestApprovalParams field '" + invalidPath + "' has the wrong type or is missing");
                return std::nullopt;
            }
            result->raw = value;
            error.clear();
            return std::move(*result);
        } catch (...) {
            fail(error, "PermissionsRequestApprovalParams could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeApplyPatchApprovalResponse(const typed::ApplyPatchApprovalResponse& value, std::string& error) noexcept {
        try {
            std::optional<Json> decision = encodeReviewDecision(value.decision, error);
            if (!decision) {
                return std::nullopt;
            }
            error.clear();
            return std::optional<Json>{Json{{"decision", std::move(*decision)}}};
        } catch (...) {
            fail(error, "ApplyPatchApprovalResponse could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeExecCommandApprovalResponse(const typed::ExecCommandApprovalResponse& value, std::string& error) noexcept {
        try {
            std::optional<Json> decision = encodeReviewDecision(value.decision, error);
            if (!decision) {
                return std::nullopt;
            }
            error.clear();
            return std::optional<Json>{Json{{"decision", std::move(*decision)}}};
        } catch (...) {
            fail(error, "ExecCommandApprovalResponse could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeCommandExecutionRequestApprovalResponse(const typed::CommandExecutionRequestApprovalResponse& value,
                                                                      std::string& error) noexcept {
        try {
            std::optional<Json> decision = encodeCommandExecutionApprovalDecision(value.decision, error);
            if (!decision) {
                return std::nullopt;
            }
            error.clear();
            return std::optional<Json>{Json{{"decision", std::move(*decision)}}};
        } catch (...) {
            fail(error, "CommandExecutionRequestApprovalResponse could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeFileChangeRequestApprovalResponse(const typed::FileChangeRequestApprovalResponse& value,
                                                                std::string& error) noexcept {
        try {
            if (value.decision.value.empty()) {
                fail(error, "FileChangeRequestApprovalResponse field '$.decision' must not be empty");
                return std::nullopt;
            }
            error.clear();
            return std::optional<Json>{Json{{"decision", value.decision.value}}};
        } catch (...) {
            fail(error, "FileChangeRequestApprovalResponse could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodePermissionsRequestApprovalResponse(const typed::PermissionsRequestApprovalResponse& value,
                                                                 std::string& error) noexcept {
        try {
            std::optional<Json> permissions = encodeGrantedPermissionProfile(value.permissions, error);
            if (!permissions) {
                return std::nullopt;
            }
            Json result{{"permissions", std::move(*permissions)}};
            if (value.scope) {
                if (value.scope->value.empty()) {
                    fail(error, "PermissionsRequestApprovalResponse field '$.scope' must not be empty");
                    return std::nullopt;
                }
                result["scope"] = value.scope->value;
            }
            if (!encodeOptionalNullable(
                    result, "strictAutoReview", value.strictAutoReview, encodeBoolean, error, "PermissionsRequestApprovalResponse")) {
                return std::nullopt;
            }
            error.clear();
            return std::optional<Json>{std::move(result)};
        } catch (...) {
            fail(error, "PermissionsRequestApprovalResponse could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodePermissionProfileListParams(const typed::PermissionProfileListParams& value, std::string& error) noexcept {
        try {
            Json result = Json::object();
            if (!encodeOptionalNullable(result, "cursor", value.cursor, encodeString, error, "PermissionProfileListParams") ||
                !encodeOptionalNullable(result, "cwd", value.cwd, encodeString, error, "PermissionProfileListParams") ||
                !encodeOptionalNullable(
                    result,
                    "limit",
                    value.limit,
                    [](const std::uint32_t& limit, std::string&) -> std::optional<Json> {
                        return std::optional<Json>{Json(limit)};
                    },
                    error,
                    "PermissionProfileListParams")) {
                return std::nullopt;
            }
            error.clear();
            return std::optional<Json>{std::move(result)};
        } catch (...) {
            fail(error, "PermissionProfileListParams could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<typed::PermissionProfileListResponse> decodePermissionProfileListResponse(const Json& value,
                                                                                            std::string& error) noexcept {
        try {
            if (!value.is_object()) {
                fail(error, "PermissionProfileListResponse at '$' must be an object");
                return std::nullopt;
            }
            const Json* data = member(value, "data");
            if (data == nullptr || !data->is_array()) {
                fail(error,
                     "PermissionProfileListResponse field '$.data' must be an "
                     "array");
                return std::nullopt;
            }

            typed::PermissionProfileListResponse result;
            result.data.reserve(data->size());
            std::size_t index = 0;
            for (const Json& rawSummary : *data) {
                if (!rawSummary.is_object()) {
                    fail(error, "PermissionProfileListResponse field '$.data[" + std::to_string(index) + "]' must be an object");
                    return std::nullopt;
                }
                typed::PermissionProfileSummary summary;
                std::string invalidPath;
                if (!decodeRequired(rawSummary, "allowed", summary.allowed, decodeBoolean, invalidPath) ||
                    !decodeOptionalNullable(rawSummary, "description", summary.description, decodeString, invalidPath) ||
                    !decodeRequired(rawSummary, "id", summary.id, decodeString, invalidPath)) {
                    fail(error,
                         "PermissionProfileListResponse field '$.data[" + std::to_string(index) + "]" + invalidPath.substr(1) +
                             "' has the wrong type or is missing");
                    return std::nullopt;
                }
                summary.raw = rawSummary;
                result.data.emplace_back(std::move(summary));
                ++index;
            }

            std::string invalidPath;
            if (!decodeOptionalNullable(value, "nextCursor", result.nextCursor, decodeString, invalidPath)) {
                fail(error, "PermissionProfileListResponse field '" + invalidPath + "' has the wrong type");
                return std::nullopt;
            }
            result.raw = value;
            error.clear();
            return result;
        } catch (...) {
            fail(error, "PermissionProfileListResponse could not be decoded");
            return std::nullopt;
        }
    }

} // namespace ai::openai::codex::detail
