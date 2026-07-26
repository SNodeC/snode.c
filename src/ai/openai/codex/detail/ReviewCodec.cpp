/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ReviewCodec.h"

#include "ai/openai/codex/detail/ApprovalCodec.h"
#include "ai/openai/codex/detail/DecodeDiagnostic.h"
#include "ai/openai/codex/detail/TurnCodec.h"
#include "ai/openai/codex/typed/ServerRequests.h"
#include "ai/openai/codex/typed/Turns.h"
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

        const Json* member(const Json& object, std::string_view name) noexcept {
            if (!object.is_object()) {
                return nullptr;
            }
            const auto iterator = object.find(name);
            return iterator == object.end() ? nullptr : &*iterator;
        }

        bool fail(std::string& error, std::string message) {
            error = std::move(message);
            return false;
        }

        typed::DecodeDiagnostic prefixedDiagnostic(typed::DecodeDiagnostic diagnostic, std::string_view prefix) {
            if (diagnostic.fieldPath.empty() || diagnostic.fieldPath == "$") {
                diagnostic.fieldPath = std::string(prefix);
            } else if (diagnostic.fieldPath.starts_with("$")) {
                diagnostic.fieldPath = std::string(prefix) + diagnostic.fieldPath.substr(1);
            }
            return diagnostic;
        }

        std::string permissionProfileFailurePath(std::string_view error) {
            // Reuse only pinned structural names. An unexpected property name
            // can itself be sensitive and must not be copied into diagnostics.
            if (error.find("'$.fileSystem") != std::string_view::npos) {
                return "$.permissions.fileSystem";
            }
            if (error.find("'$.network") != std::string_view::npos) {
                return "$.permissions.network";
            }
            return "$.permissions";
        }

        void prefixErrorPath(std::string& error, std::string_view prefix) {
            const std::size_t marker = error.find("'$");
            if (marker != std::string::npos) {
                error.replace(marker + 1, 1, prefix);
            }
        }

        void appendDiagnostics(std::vector<typed::DecodeDiagnostic>& target,
                               const std::vector<typed::DecodeDiagnostic>& source,
                               std::string_view prefix) {
            for (const typed::DecodeDiagnostic& diagnostic : source) {
                target.emplace_back(prefixedDiagnostic(diagnostic, prefix));
            }
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

        bool decodeUint16(const Json& value, std::uint16_t& result) noexcept {
            std::uint64_t number = 0;
            if (value.is_number_unsigned()) {
                number = value.get_ref<const Json::number_unsigned_t&>();
            } else if (value.is_number_integer()) {
                const auto signedNumber = value.get_ref<const Json::number_integer_t&>();
                if (signedNumber < 0) {
                    return false;
                }
                number = static_cast<std::uint64_t>(signedNumber);
            } else {
                return false;
            }
            if (number > std::numeric_limits<std::uint16_t>::max()) {
                return false;
            }
            result = static_cast<std::uint16_t>(number);
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

        bool decodeAbsolutePathArray(const Json& value, std::vector<typed::AbsolutePathBuf>& result) {
            if (!value.is_array()) {
                return false;
            }
            std::vector<typed::AbsolutePathBuf> decoded;
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
            if (value == nullptr || !decode(*value, result)) {
                invalidPath = "$." + std::string(name);
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
            if (!decode(*value, decoded)) {
                invalidPath = "$." + std::string(name);
                return false;
            }
            result = typed::OptionalNullable<T>::withValue(std::move(decoded));
            return true;
        }

        template <typename OpenEnum>
        bool decodeOpenEnum(const Json& value,
                            OpenEnum& result,
                            std::vector<typed::DecodeDiagnostic>& diagnostics,
                            std::string_view surface,
                            std::string path) {
            if (!decodeString(value, result.value)) {
                return false;
            }
            if (!result.isKnown()) {
                diagnostics.emplace_back(unknownEnumDiagnostic(std::string(surface), std::move(path)));
            }
            return true;
        }

        template <typename T>
        ConversationDecodeResult<T> malformedUnion(std::string_view surface, std::string path) {
            return {std::nullopt, malformedKnownDiagnostic(std::string(surface), std::move(path))};
        }

        template <typename Union, typename Unknown>
        ConversationDecodeResult<Union>
        malformedPreserved(const Json& value, std::optional<std::string> discriminator, std::string_view surface, std::string path) {
            typed::DecodeDiagnostic diagnostic = malformedKnownDiagnostic(std::string(surface), std::move(path));
            return {Union{Unknown{std::move(discriminator), value, diagnostic}}, std::move(diagnostic)};
        }

        template <typename Union, typename Alternative>
        ConversationDecodeResult<Union> decodedUnion(Alternative value, std::optional<typed::DecodeDiagnostic> diagnostic = std::nullopt) {
            return {Union{std::move(value)}, std::move(diagnostic)};
        }

        template <typename Union, typename Unknown>
        ConversationDecodeResult<Union>
        unknownUnion(const Json& value, std::optional<std::string> discriminator, std::string_view surface) {
            typed::DecodeDiagnostic diagnostic = unknownDiscriminatorDiagnostic(std::string(surface), "$.type");
            return {Union{Unknown{std::move(discriminator), value, diagnostic}}, std::move(diagnostic)};
        }

        template <typename T, typename Encode>
        bool encodeOptionalNullable(
            Json& object, std::string_view name, const typed::OptionalNullable<T>& value, Encode&& encode, std::string& error) {
            if (!value.present && value.value.has_value()) {
                return fail(error, "review field '$." + std::string(name) + "' has an inconsistent omitted state");
            }
            if (value.isOmitted()) {
                return true;
            }
            if (value.isNull()) {
                object[std::string(name)] = nullptr;
                return true;
            }
            object[std::string(name)] = encode(*value);
            return true;
        }

        bool decodeGuardianApprovalReview(const Json& value,
                                          typed::GuardianApprovalReview& result,
                                          std::vector<typed::DecodeDiagnostic>& parentDiagnostics,
                                          std::string& error) {
            constexpr std::string_view Context = "GuardianApprovalReview";
            if (!value.is_object()) {
                return fail(error, "GuardianApprovalReview at '$' must be an object");
            }

            const Json* status = member(value, "status");
            if (status == nullptr ||
                !decodeOpenEnum(*status, result.status, result.diagnostics, "GuardianApprovalReviewStatus", "$.status")) {
                return fail(error, "GuardianApprovalReview field '$.status' must be a string");
            }

            std::string invalidPath;
            if (!decodeOptionalNullable(value, "rationale", result.rationale, decodeString, invalidPath) ||
                !decodeOptionalNullable(
                    value,
                    "riskLevel",
                    result.riskLevel,
                    [&](const Json& nested, typed::GuardianRiskLevel& decoded) {
                        return decodeOpenEnum(nested, decoded, result.diagnostics, "GuardianRiskLevel", "$.riskLevel");
                    },
                    invalidPath) ||
                !decodeOptionalNullable(
                    value,
                    "userAuthorization",
                    result.userAuthorization,
                    [&](const Json& nested, typed::GuardianUserAuthorization& decoded) {
                        return decodeOpenEnum(nested, decoded, result.diagnostics, "GuardianUserAuthorization", "$.userAuthorization");
                    },
                    invalidPath)) {
                return fail(error, std::string(Context) + " field '" + invalidPath + "' has the wrong type");
            }

            result.raw = value;
            appendDiagnostics(parentDiagnostics, result.diagnostics, "$.params.review");
            return true;
        }

        bool decodeActionPreservingMalformed(const Json& value,
                                             typed::GuardianApprovalReviewAction& result,
                                             std::vector<typed::DecodeDiagnostic>& diagnostics) {
            auto decoded = decodeGuardianApprovalReviewAction(value);
            if (decoded.value) {
                result = std::move(*decoded.value);
                std::visit(
                    [&](const auto& alternative) {
                        using Alternative = std::decay_t<decltype(alternative)>;
                        if constexpr (std::is_same_v<Alternative, typed::UnknownGuardianApprovalReviewAction>) {
                            diagnostics.emplace_back(prefixedDiagnostic(alternative.diagnostic, "$.params.action"));
                        } else {
                            appendDiagnostics(diagnostics, alternative.diagnostics, "$.params.action");
                        }
                    },
                    result);
                return true;
            }

            typed::DecodeDiagnostic diagnostic = decoded.diagnostic.value_or(malformedKnownDiagnostic("GuardianApprovalReviewAction", "$"));
            std::optional<std::string> type;
            const Json* typeValue = member(value, "type");
            if (typeValue != nullptr && typeValue->is_string()) {
                type = typeValue->get_ref<const std::string&>();
            }
            result = typed::UnknownGuardianApprovalReviewAction{std::move(type), value, diagnostic};
            diagnostics.emplace_back(prefixedDiagnostic(std::move(diagnostic), "$.params.action"));
            return true;
        }

        template <typename NotificationType>
        bool decodeGuardianReviewNotificationCommon(const Notification& notification, NotificationType& result, std::string& error) {
            if (!notification.params.is_object()) {
                return fail(error,
                            "guardian approval review notification params at '$.params' "
                            "must be an object");
            }

            const Json* action = member(notification.params, "action");
            const Json* review = member(notification.params, "review");
            std::string invalidPath;
            if (action == nullptr) {
                return fail(error,
                            "guardian approval review notification field "
                            "'$.params.action' is missing");
            }
            if (review == nullptr || !decodeGuardianApprovalReview(*review, result.review, result.diagnostics, error)) {
                if (error.empty()) {
                    fail(error,
                         "guardian approval review notification field "
                         "'$.params.review' is missing");
                } else {
                    prefixErrorPath(error, "$.params.review");
                }
                return false;
            }
            if (!decodeActionPreservingMalformed(*action, result.action, result.diagnostics) ||
                !decodeRequired(notification.params, "reviewId", result.reviewId, decodeString, invalidPath) ||
                !decodeRequired(notification.params, "startedAtMs", result.startedAtMs, decodeInt64, invalidPath) ||
                !decodeOptionalNullable(
                    notification.params, "targetItemId", result.targetItemId, decodeStrongString<typed::ItemId>, invalidPath) ||
                !decodeRequired(notification.params, "threadId", result.threadId, decodeStrongString<typed::ThreadId>, invalidPath) ||
                !decodeRequired(notification.params, "turnId", result.turnId, decodeStrongString<typed::TurnId>, invalidPath)) {
                return fail(error,
                            "guardian approval review notification field "
                            "'$.params" +
                                invalidPath.substr(1) + "' has the wrong type or is missing");
            }
            result.raw = notification.raw;
            return true;
        }

    } // namespace

    ConversationDecodeResult<typed::ReviewTarget> decodeReviewTarget(const Json& value) noexcept {
        constexpr std::string_view Surface = "ReviewTarget";
        try {
            if (!value.is_object()) {
                return malformedPreserved<typed::ReviewTarget, typed::UnknownReviewTarget>(value, std::nullopt, Surface, "$");
            }
            const Json* typeValue = member(value, "type");
            if (typeValue == nullptr || !typeValue->is_string()) {
                return malformedPreserved<typed::ReviewTarget, typed::UnknownReviewTarget>(value, std::nullopt, Surface, "$.type");
            }
            const std::string type = typeValue->get_ref<const std::string&>();
            if (type == "uncommittedChanges") {
                return decodedUnion<typed::ReviewTarget>(typed::UncommittedChangesReviewTarget{});
            }

            std::string invalidPath;
            if (type == "baseBranch") {
                typed::BaseBranchReviewTarget result;
                if (!decodeRequired(value, "branch", result.branch, decodeString, invalidPath)) {
                    return malformedPreserved<typed::ReviewTarget, typed::UnknownReviewTarget>(
                        value, type, Surface, std::move(invalidPath));
                }
                return decodedUnion<typed::ReviewTarget>(std::move(result));
            }
            if (type == "commit") {
                typed::CommitReviewTarget result;
                if (!decodeRequired(value, "sha", result.sha, decodeString, invalidPath) ||
                    !decodeOptionalNullable(value, "title", result.title, decodeString, invalidPath)) {
                    return malformedPreserved<typed::ReviewTarget, typed::UnknownReviewTarget>(
                        value, type, Surface, std::move(invalidPath));
                }
                return decodedUnion<typed::ReviewTarget>(std::move(result));
            }
            if (type == "custom") {
                typed::CustomReviewTarget result;
                if (!decodeRequired(value, "instructions", result.instructions, decodeString, invalidPath)) {
                    return malformedPreserved<typed::ReviewTarget, typed::UnknownReviewTarget>(
                        value, type, Surface, std::move(invalidPath));
                }
                return decodedUnion<typed::ReviewTarget>(std::move(result));
            }
            return unknownUnion<typed::ReviewTarget, typed::UnknownReviewTarget>(value, type, Surface);
        } catch (...) {
            return malformedUnion<typed::ReviewTarget>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::GuardianApprovalReviewAction> decodeGuardianApprovalReviewAction(const Json& value) noexcept {
        constexpr std::string_view Surface = "GuardianApprovalReviewAction";
        try {
            if (!value.is_object()) {
                return malformedPreserved<typed::GuardianApprovalReviewAction, typed::UnknownGuardianApprovalReviewAction>(
                    value, std::nullopt, Surface, "$");
            }
            const Json* typeValue = member(value, "type");
            if (typeValue == nullptr || !typeValue->is_string()) {
                return malformedPreserved<typed::GuardianApprovalReviewAction, typed::UnknownGuardianApprovalReviewAction>(
                    value, std::nullopt, Surface, "$.type");
            }
            const std::string type = typeValue->get_ref<const std::string&>();
            std::string invalidPath;

            if (type == "command") {
                typed::CommandGuardianApprovalReviewAction result;
                if (!decodeRequired(value, "command", result.command, decodeString, invalidPath) ||
                    !decodeRequired(value, "cwd", result.cwd, decodeStrongString<typed::AbsolutePathBuf>, invalidPath) ||
                    !decodeRequired(
                        value,
                        "source",
                        result.source,
                        [&](const Json& nested, typed::GuardianCommandSource& decoded) {
                            return decodeOpenEnum(nested, decoded, result.diagnostics, "GuardianCommandSource", "$.source");
                        },
                        invalidPath)) {
                    return malformedPreserved<typed::GuardianApprovalReviewAction, typed::UnknownGuardianApprovalReviewAction>(
                        value, type, Surface, std::move(invalidPath));
                }
                result.raw = value;
                const std::optional<typed::DecodeDiagnostic> diagnostic =
                    result.diagnostics.empty() ? std::nullopt : std::optional<typed::DecodeDiagnostic>{result.diagnostics.front()};
                return decodedUnion<typed::GuardianApprovalReviewAction>(std::move(result), diagnostic);
            }

            if (type == "execve") {
                typed::ExecveGuardianApprovalReviewAction result;
                if (!decodeRequired(value, "argv", result.argv, decodeStringArray, invalidPath) ||
                    !decodeRequired(value, "cwd", result.cwd, decodeStrongString<typed::AbsolutePathBuf>, invalidPath) ||
                    !decodeRequired(value, "program", result.program, decodeString, invalidPath) ||
                    !decodeRequired(
                        value,
                        "source",
                        result.source,
                        [&](const Json& nested, typed::GuardianCommandSource& decoded) {
                            return decodeOpenEnum(nested, decoded, result.diagnostics, "GuardianCommandSource", "$.source");
                        },
                        invalidPath)) {
                    return malformedPreserved<typed::GuardianApprovalReviewAction, typed::UnknownGuardianApprovalReviewAction>(
                        value, type, Surface, std::move(invalidPath));
                }
                result.raw = value;
                const std::optional<typed::DecodeDiagnostic> diagnostic =
                    result.diagnostics.empty() ? std::nullopt : std::optional<typed::DecodeDiagnostic>{result.diagnostics.front()};
                return decodedUnion<typed::GuardianApprovalReviewAction>(std::move(result), diagnostic);
            }

            if (type == "applyPatch") {
                typed::ApplyPatchGuardianApprovalReviewAction result;
                if (!decodeRequired(value, "cwd", result.cwd, decodeStrongString<typed::AbsolutePathBuf>, invalidPath) ||
                    !decodeRequired(value, "files", result.files, decodeAbsolutePathArray, invalidPath)) {
                    return malformedPreserved<typed::GuardianApprovalReviewAction, typed::UnknownGuardianApprovalReviewAction>(
                        value, type, Surface, std::move(invalidPath));
                }
                result.raw = value;
                return decodedUnion<typed::GuardianApprovalReviewAction>(std::move(result));
            }

            if (type == "networkAccess") {
                typed::NetworkAccessGuardianApprovalReviewAction result;
                if (!decodeRequired(value, "host", result.host, decodeString, invalidPath) ||
                    !decodeRequired(value, "port", result.port, decodeUint16, invalidPath) ||
                    !decodeRequired(
                        value,
                        "protocol",
                        result.protocol,
                        [&](const Json& nested, typed::NetworkApprovalProtocol& decoded) {
                            return decodeOpenEnum(nested, decoded, result.diagnostics, "NetworkApprovalProtocol", "$.protocol");
                        },
                        invalidPath) ||
                    !decodeRequired(value, "target", result.target, decodeString, invalidPath)) {
                    return malformedPreserved<typed::GuardianApprovalReviewAction, typed::UnknownGuardianApprovalReviewAction>(
                        value, type, Surface, std::move(invalidPath));
                }
                result.raw = value;
                const std::optional<typed::DecodeDiagnostic> diagnostic =
                    result.diagnostics.empty() ? std::nullopt : std::optional<typed::DecodeDiagnostic>{result.diagnostics.front()};
                return decodedUnion<typed::GuardianApprovalReviewAction>(std::move(result), diagnostic);
            }

            if (type == "mcpToolCall") {
                typed::McpToolCallGuardianApprovalReviewAction result;
                if (!decodeOptionalNullable(value, "connectorId", result.connectorId, decodeString, invalidPath) ||
                    !decodeOptionalNullable(value, "connectorName", result.connectorName, decodeString, invalidPath) ||
                    !decodeRequired(value, "server", result.server, decodeString, invalidPath) ||
                    !decodeRequired(value, "toolName", result.toolName, decodeString, invalidPath) ||
                    !decodeOptionalNullable(value, "toolTitle", result.toolTitle, decodeString, invalidPath)) {
                    return malformedPreserved<typed::GuardianApprovalReviewAction, typed::UnknownGuardianApprovalReviewAction>(
                        value, type, Surface, std::move(invalidPath));
                }
                result.raw = value;
                return decodedUnion<typed::GuardianApprovalReviewAction>(std::move(result));
            }

            if (type == "requestPermissions") {
                typed::RequestPermissionsGuardianApprovalReviewAction result;
                const Json* permissions = member(value, "permissions");
                std::string permissionError;
                if (permissions == nullptr) {
                    return malformedPreserved<typed::GuardianApprovalReviewAction, typed::UnknownGuardianApprovalReviewAction>(
                        value, type, Surface, "$.permissions");
                }
                auto decodedPermissions = decodeRequestPermissionProfile(*permissions, permissionError);
                if (!decodedPermissions) {
                    return malformedPreserved<typed::GuardianApprovalReviewAction, typed::UnknownGuardianApprovalReviewAction>(
                        value, type, Surface, permissionProfileFailurePath(permissionError));
                }
                result.permissions = std::move(*decodedPermissions);
                appendDiagnostics(result.diagnostics, result.permissions.diagnostics, "$.permissions");
                if (!decodeOptionalNullable(value, "reason", result.reason, decodeString, invalidPath)) {
                    return malformedPreserved<typed::GuardianApprovalReviewAction, typed::UnknownGuardianApprovalReviewAction>(
                        value, type, Surface, std::move(invalidPath));
                }
                result.raw = value;
                const std::optional<typed::DecodeDiagnostic> diagnostic =
                    result.diagnostics.empty() ? std::nullopt : std::optional<typed::DecodeDiagnostic>{result.diagnostics.front()};
                return decodedUnion<typed::GuardianApprovalReviewAction>(std::move(result), diagnostic);
            }

            return unknownUnion<typed::GuardianApprovalReviewAction, typed::UnknownGuardianApprovalReviewAction>(value, type, Surface);
        } catch (...) {
            return malformedUnion<typed::GuardianApprovalReviewAction>(Surface, "$");
        }
    }

    std::optional<Json> encodeReviewTarget(const typed::ReviewTarget& value, std::string& error) noexcept {
        try {
            error.clear();
            return std::visit(
                [&](const auto& alternative) -> std::optional<Json> {
                    using Alternative = std::decay_t<decltype(alternative)>;
                    if constexpr (std::is_same_v<Alternative, typed::UncommittedChangesReviewTarget>) {
                        return std::optional<Json>{Json{{"type", "uncommittedChanges"}}};
                    } else if constexpr (std::is_same_v<Alternative, typed::BaseBranchReviewTarget>) {
                        return std::optional<Json>{Json{{"type", "baseBranch"}, {"branch", alternative.branch}}};
                    } else if constexpr (std::is_same_v<Alternative, typed::CommitReviewTarget>) {
                        Json result{{"type", "commit"}, {"sha", alternative.sha}};
                        if (!encodeOptionalNullable(
                                result,
                                "title",
                                alternative.title,
                                [](const std::string& nested) {
                                    return nested;
                                },
                                error)) {
                            return std::nullopt;
                        }
                        return std::optional<Json>{std::move(result)};
                    } else if constexpr (std::is_same_v<Alternative, typed::CustomReviewTarget>) {
                        return std::optional<Json>{Json{{"type", "custom"}, {"instructions", alternative.instructions}}};
                    } else {
                        fail(error,
                             "unknown ReviewTarget alternative cannot be encoded; "
                             "use the raw protocol API");
                        return std::nullopt;
                    }
                },
                value);
        } catch (...) {
            fail(error, "ReviewTarget could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeReviewStartParams(const typed::ReviewStartParams& value, std::string& error) noexcept {
        try {
            auto target = encodeReviewTarget(value.target, error);
            if (!target) {
                return std::nullopt;
            }
            Json result{{"threadId", value.threadId.value}, {"target", std::move(*target)}};
            if (!encodeOptionalNullable(
                    result,
                    "delivery",
                    value.delivery,
                    [](const typed::ReviewDelivery& nested) {
                        return nested.value;
                    },
                    error)) {
                return std::nullopt;
            }
            error.clear();
            return std::optional<Json>{std::move(result)};
        } catch (...) {
            fail(error, "review/start parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<Json> encodeThreadApproveGuardianDeniedActionParams(const typed::ThreadApproveGuardianDeniedActionParams& value,
                                                                      std::string& error) noexcept {
        try {
            error.clear();
            return std::optional<Json>{Json{{"threadId", value.threadId.value}, {"event", value.event}}};
        } catch (...) {
            fail(error, "thread/approveGuardianDeniedAction parameters could not be encoded");
            return std::nullopt;
        }
    }

    std::optional<typed::ReviewStartResponse> decodeReviewStartResponse(const Json& value, std::string& error) noexcept {
        try {
            if (!value.is_object()) {
                fail(error, "ReviewStartResponse at '$' must be an object");
                return std::nullopt;
            }
            const Json* reviewThreadId = member(value, "reviewThreadId");
            const Json* turn = member(value, "turn");
            if (reviewThreadId == nullptr || !reviewThreadId->is_string()) {
                fail(error, "ReviewStartResponse field '$.reviewThreadId' must be a string");
                return std::nullopt;
            }
            if (turn == nullptr) {
                fail(error, "ReviewStartResponse field '$.turn' is missing");
                return std::nullopt;
            }

            typed::ReviewStartResponse result;
            result.reviewThreadId.value = reviewThreadId->get_ref<const std::string&>();
            auto decodedTurn = decodeTurn(*turn, result.reviewThreadId, error);
            if (!decodedTurn) {
                if (error.empty()) {
                    fail(error, "ReviewStartResponse field '$.turn' does not match Turn");
                } else {
                    error = "ReviewStartResponse field '$.turn': " + error;
                }
                return std::nullopt;
            }
            if (hasMalformedKnownPayload(*decodedTurn)) {
                fail(error,
                     "ReviewStartResponse field '$.turn' contains a malformed "
                     "known protocol payload");
                return std::nullopt;
            }
            result.turn = std::move(*decodedTurn);
            result.raw = value;
            error.clear();
            return std::optional<typed::ReviewStartResponse>{std::move(result)};
        } catch (...) {
            fail(error, "ReviewStartResponse could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::GuardianWarningNotification> decodeGuardianWarningNotification(const Notification& notification,
                                                                                        std::string& error) noexcept {
        try {
            if (!notification.params.is_object()) {
                fail(error, "guardianWarning params at '$.params' must be an object");
                return std::nullopt;
            }
            typed::GuardianWarningNotification result;
            std::string invalidPath;
            if (!decodeRequired(notification.params, "message", result.message, decodeString, invalidPath) ||
                !decodeRequired(notification.params, "threadId", result.threadId, decodeStrongString<typed::ThreadId>, invalidPath)) {
                fail(error, "guardianWarning field '$.params" + invalidPath.substr(1) + "' has the wrong type or is missing");
                return std::nullopt;
            }
            result.raw = notification.raw;
            error.clear();
            return std::optional<typed::GuardianWarningNotification>{std::move(result)};
        } catch (...) {
            fail(error, "guardianWarning could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::ItemGuardianApprovalReviewStartedNotification>
    decodeItemGuardianApprovalReviewStartedNotification(const Notification& notification, std::string& error) noexcept {
        try {
            typed::ItemGuardianApprovalReviewStartedNotification result;
            if (!decodeGuardianReviewNotificationCommon(notification, result, error)) {
                return std::nullopt;
            }
            error.clear();
            return std::optional<typed::ItemGuardianApprovalReviewStartedNotification>{std::move(result)};
        } catch (...) {
            fail(error, "item/autoApprovalReview/started could not be decoded");
            return std::nullopt;
        }
    }

    std::optional<typed::ItemGuardianApprovalReviewCompletedNotification>
    decodeItemGuardianApprovalReviewCompletedNotification(const Notification& notification, std::string& error) noexcept {
        try {
            typed::ItemGuardianApprovalReviewCompletedNotification result;
            if (!decodeGuardianReviewNotificationCommon(notification, result, error)) {
                return std::nullopt;
            }
            std::string invalidPath;
            if (!decodeRequired(notification.params, "completedAtMs", result.completedAtMs, decodeInt64, invalidPath) ||
                !decodeRequired(
                    notification.params,
                    "decisionSource",
                    result.decisionSource,
                    [&](const Json& nested, typed::AutoReviewDecisionSource& decoded) {
                        return decodeOpenEnum(nested, decoded, result.diagnostics, "AutoReviewDecisionSource", "$.params.decisionSource");
                    },
                    invalidPath)) {
                fail(error,
                     "item/autoApprovalReview/completed field '$.params" + invalidPath.substr(1) + "' has the wrong type or is missing");
                return std::nullopt;
            }
            result.raw = notification.raw;
            error.clear();
            return std::optional<typed::ItemGuardianApprovalReviewCompletedNotification>{std::move(result)};
        } catch (...) {
            fail(error, "item/autoApprovalReview/completed could not be decoded");
            return std::nullopt;
        }
    }

} // namespace ai::openai::codex::detail
