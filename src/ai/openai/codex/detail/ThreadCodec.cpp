/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ThreadCodec.h"

#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/detail/TurnCodec.h"
#include "ai/openai/codex/typed/Items.h"
#include "ai/openai/codex/typed/Turns.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <map>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace ai::openai::codex::detail {

    namespace {

        constexpr std::string_view VariantField = "$variant";
        constexpr std::string_view TypeField = "type";

        const Json* member(const Json& object, const std::string_view name) noexcept {
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

        typed::DecodeDiagnostic malformedDiagnostic(std::string surface, std::string path) {
            return {typed::DecodeIssueKind::MalformedKnownPayload,
                    typed::DecodeIssueSeverity::ProtocolWarning,
                    std::move(surface),
                    std::move(path),
                    "known protocol surface contains an invalid value"};
        }

        typed::DecodeDiagnostic unknownDiscriminatorDiagnostic(std::string surface, std::string path) {
            return {typed::DecodeIssueKind::UnknownDiscriminator,
                    typed::DecodeIssueSeverity::ForwardCompatibility,
                    std::move(surface),
                    std::move(path),
                    "protocol surface contains a future discriminator"};
        }

        typed::DecodeDiagnostic unknownEnumDiagnostic(std::string surface, std::string path) {
            return {typed::DecodeIssueKind::UnknownEnumValue,
                    typed::DecodeIssueSeverity::ForwardCompatibility,
                    std::move(surface),
                    std::move(path),
                    "protocol surface contains a future string-enum value"};
        }

        struct UnionResolution {
            const ProtocolSurfaceEntry* entry = nullptr;
            const ConversationUnionCodecDescriptor* descriptor = nullptr;
            std::optional<typed::DecodeDiagnostic> diagnostic;
        };

        UnionResolution
        resolveUnion(std::string_view surface, std::string_view field, std::string_view discriminator, std::string path) {
            UnionResolution result;
            result.entry =
                findSurface(SurfaceCategory::TaggedUnionDiscriminator, surface, field, discriminator);
            if (result.entry == nullptr) {
                return result;
            }
            if (result.entry->runtimeDisposition != RuntimeDisposition::Typed ||
                result.entry->typedImplementation != TypedImplementationStatus::Implemented) {
                result.diagnostic = malformedDiagnostic(std::string(surface), std::move(path));
                return result;
            }
            const auto* target = std::get_if<ConversationUnionTarget>(&result.entry->runtimeTarget);
            if (target == nullptr) {
                result.diagnostic = malformedDiagnostic(std::string(surface), std::move(path));
                return result;
            }
            const auto descriptors = conversationUnionCodecDescriptors();
            const auto iterator =
                std::find_if(descriptors.begin(), descriptors.end(), [&](const ConversationUnionCodecDescriptor& descriptor) {
                    return descriptor.key == result.entry->key && descriptor.target == *target;
                });
            if (iterator == descriptors.end() || iterator->direction != ConversationUnionCodecDirection::DecodeOnly) {
                result.diagnostic = malformedDiagnostic(std::string(surface), std::move(path));
                return result;
            }
            result.descriptor = &*iterator;
            return result;
        }

        template <typename T>
        ConversationDecodeResult<T> malformedUnion(std::string_view surface, std::string path) {
            auto diagnostic = malformedDiagnostic(std::string(surface), std::move(path));
            return {std::nullopt, std::move(diagnostic)};
        }

        bool decodeInt32(const Json& value, std::int32_t& result) noexcept {
            if (value.is_number_unsigned()) {
                const auto number = value.get_ref<const Json::number_unsigned_t&>();
                if (number > static_cast<Json::number_unsigned_t>(std::numeric_limits<std::int32_t>::max())) {
                    return false;
                }
                result = static_cast<std::int32_t>(number);
                return true;
            }
            if (!value.is_number_integer()) {
                return false;
            }
            const auto number = value.get_ref<const Json::number_integer_t&>();
            if (number < std::numeric_limits<std::int32_t>::min() || number > std::numeric_limits<std::int32_t>::max()) {
                return false;
            }
            result = static_cast<std::int32_t>(number);
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

        bool requiredString(const Json& object,
                            std::string_view name,
                            std::string& result,
                            std::string& error,
                            std::string_view context) {
            const Json* value = member(object, name);
            if (value == nullptr || !value->is_string()) {
                return fail(error, std::string(context) + " field '" + std::string(name) + "' must be a string");
            }
            result = value->get_ref<const std::string&>();
            return true;
        }

        bool requiredBool(const Json& object,
                          std::string_view name,
                          bool& result,
                          std::string& error,
                          std::string_view context) {
            const Json* value = member(object, name);
            if (value == nullptr || !value->is_boolean()) {
                return fail(error, std::string(context) + " field '" + std::string(name) + "' must be a boolean");
            }
            result = value->get_ref<const Json::boolean_t&>();
            return true;
        }

        bool requiredInt64(const Json& object,
                           std::string_view name,
                           std::int64_t& result,
                           std::string& error,
                           std::string_view context) {
            const Json* value = member(object, name);
            if (value == nullptr || !decodeInt64(*value, result)) {
                return fail(error, std::string(context) + " field '" + std::string(name) + "' must be an int64 integer");
            }
            return true;
        }

        template <typename T, typename Decode>
        bool decodeOptionalNullable(const Json& object,
                                    std::string_view name,
                                    typed::OptionalNullable<T>& result,
                                    Decode&& decode,
                                    std::string& error,
                                    std::string_view context) {
            result = typed::OptionalNullable<T>::omitted();
            const Json* value = member(object, name);
            if (value == nullptr) {
                return true;
            }
            result.present = true;
            if (value->is_null()) {
                return true;
            }
            T decoded;
            if (!decode(*value, decoded)) {
                return fail(error,
                            std::string(context) + " field '" + std::string(name) +
                                "' has the wrong type");
            }
            result.value = std::move(decoded);
            return true;
        }

        bool decodeString(const Json& value, std::string& result) {
            if (!value.is_string()) {
                return false;
            }
            result = value.get_ref<const std::string&>();
            return true;
        }

        bool decodeThreadId(const Json& value, typed::ThreadId& result) {
            std::string stringValue;
            if (!decodeString(value, stringValue)) {
                return false;
            }
            result.value = std::move(stringValue);
            return true;
        }

        bool decodeThreadSource(const Json& value, typed::ThreadSource& result) {
            std::string stringValue;
            if (!decodeString(value, stringValue)) {
                return false;
            }
            result.value = std::move(stringValue);
            return true;
        }

        bool decodeGitInfoValue(const Json& value, typed::GitInfo& result, std::string& error) {
            if (!value.is_object()) {
                return false;
            }
            if (!decodeOptionalNullable(value, "branch", result.branch, decodeString, error, "GitInfo") ||
                !decodeOptionalNullable(value, "originUrl", result.originUrl, decodeString, error, "GitInfo") ||
                !decodeOptionalNullable(value, "sha", result.sha, decodeString, error, "GitInfo")) {
                return false;
            }
            result.raw = value;
            return true;
        }

        template <typename T, typename Encode>
        bool encodeOptionalNullable(Json& object,
                                    std::string_view name,
                                    const typed::OptionalNullable<T>& value,
                                    Encode&& encode,
                                    std::string& error) {
            if (value.isOmitted()) {
                return true;
            }
            if (value.isNull()) {
                object[std::string(name)] = nullptr;
                return true;
            }
            std::optional<Json> encoded = encode(*value.value, error);
            if (!encoded) {
                return false;
            }
            object[std::string(name)] = std::move(*encoded);
            return true;
        }

        template <typename T>
        std::optional<Json> scalarValue(const T& value, std::string&) {
            return std::optional<Json>{std::in_place, value};
        }

        template <typename T>
        std::optional<Json> openValue(const T& value, std::string&) {
            return std::optional<Json>{std::in_place, value.value};
        }

        std::optional<Json> encodeConfiguration(const typed::ProtocolConfiguration& configuration, std::string&) {
            Json result = Json::object();
            for (const auto& [key, value] : configuration) {
                result[key] = value;
            }
            return std::optional<Json>{std::in_place, std::move(result)};
        }

        std::optional<Json> encodeThreadSourceValue(const typed::ThreadSource& value, std::string&) {
            return std::optional<Json>{std::in_place, value.value};
        }

        template <typename Params>
        std::optional<Json> encodeThreadIdOnly(const Params& params, std::string& error, std::string_view context) {
            try {
                error.clear();
                return std::optional<Json>{std::in_place, Json::object({{"threadId", params.threadId.value}})};
            } catch (const std::exception& exception) {
                error = std::string("failed to encode ") + std::string(context) + " parameters: " + exception.what();
            } catch (...) {
                error = std::string("failed to encode ") + std::string(context) + " parameters";
            }
            return std::nullopt;
        }

        template <typename Params>
        bool encodeThreadOverrides(Json& result, const Params& params, std::string& error) {
            return encodeOptionalNullable(result, "approvalPolicy", params.approvalPolicy, encodeAskForApproval, error) &&
                   encodeOptionalNullable(result, "approvalsReviewer", params.approvalsReviewer, openValue<typed::ApprovalsReviewer>, error) &&
                   encodeOptionalNullable(result, "baseInstructions", params.baseInstructions, scalarValue<std::string>, error) &&
                   encodeOptionalNullable(result, "config", params.config, encodeConfiguration, error) &&
                   encodeOptionalNullable(result, "cwd", params.cwd, scalarValue<std::string>, error) &&
                   encodeOptionalNullable(result, "developerInstructions", params.developerInstructions, scalarValue<std::string>, error) &&
                   encodeOptionalNullable(result, "serviceTier", params.serviceTier, scalarValue<std::string>, error) &&
                   encodeOptionalNullable(result, "model", params.model, openValue<typed::ModelId>, error) &&
                   encodeOptionalNullable(result, "modelProvider", params.modelProvider, scalarValue<std::string>, error) &&
                   encodeOptionalNullable(result, "sandbox", params.sandbox, openValue<typed::SandboxMode>, error);
        }

        template <typename T>
        void appendDiagnostic(std::vector<typed::DecodeDiagnostic>& target, const ConversationDecodeResult<T>& decoded) {
            if (decoded.diagnostic) {
                target.emplace_back(*decoded.diagnostic);
            }
        }

        std::optional<typed::ThreadGoal> decodeThreadGoal(const Json& value, std::string& error) {
            if (!value.is_object()) {
                fail(error, "ThreadGoal must be an object");
                return std::nullopt;
            }
            typed::ThreadGoal result;
            std::string threadId;
            std::string status;
            if (!requiredInt64(value, "createdAt", result.createdAt, error, "ThreadGoal") ||
                !requiredString(value, "objective", result.objective, error, "ThreadGoal") ||
                !requiredString(value, "status", status, error, "ThreadGoal") ||
                !requiredString(value, "threadId", threadId, error, "ThreadGoal") ||
                !requiredInt64(value, "timeUsedSeconds", result.timeUsedSeconds, error, "ThreadGoal") ||
                !decodeOptionalNullable(value, "tokenBudget", result.tokenBudget, decodeInt64, error, "ThreadGoal") ||
                !requiredInt64(value, "tokensUsed", result.tokensUsed, error, "ThreadGoal") ||
                !requiredInt64(value, "updatedAt", result.updatedAt, error, "ThreadGoal")) {
                return std::nullopt;
            }
            result.threadId.value = std::move(threadId);
            result.status.value = std::move(status);
            if (!result.status.isKnown()) {
                result.diagnostics.emplace_back(unknownEnumDiagnostic("ThreadGoalStatus", "$.status"));
            }
            result.raw = value;
            return result;
        }

        template <typename Response>
        std::optional<Response> decodeThreadLaunchResponse(const Json& value, std::string& error, std::string_view context) {
            if (!value.is_object()) {
                fail(error, std::string(context) + " must be an object");
                return std::nullopt;
            }
            Response result;
            const Json* approvalPolicy = member(value, "approvalPolicy");
            const Json* sandbox = member(value, "sandbox");
            const Json* thread = member(value, "thread");
            std::string reviewer;
            std::string cwd;
            std::string model;
            if (approvalPolicy == nullptr || sandbox == nullptr || thread == nullptr ||
                !requiredString(value, "approvalsReviewer", reviewer, error, context) ||
                !requiredString(value, "cwd", cwd, error, context) ||
                !requiredString(value, "model", model, error, context) ||
                !requiredString(value, "modelProvider", result.modelProvider, error, context)) {
                if (error.empty()) {
                    fail(error, std::string(context) + " is missing a required aggregate field");
                }
                return std::nullopt;
            }
            auto approval = decodeAskForApproval(*approvalPolicy);
            auto sandboxPolicy = decodeSandboxPolicy(*sandbox);
            if (!approval.value) {
                fail(error, std::string(context) + " field 'approvalPolicy' is malformed");
                return std::nullopt;
            }
            if (!sandboxPolicy.value) {
                fail(error, std::string(context) + " field 'sandbox' is malformed");
                return std::nullopt;
            }
            std::optional<typed::Thread> decodedThread = decodeThread(*thread, error);
            if (!decodedThread) {
                error = std::string(context) + " field 'thread': " + error;
                return std::nullopt;
            }
            if (std::any_of(
                    decodedThread->turns.begin(), decodedThread->turns.end(), hasMalformedKnownPayload)) {
                fail(error, std::string(context) + " field 'thread' contains a malformed known payload");
                return std::nullopt;
            }
            result.approvalPolicy = std::move(*approval.value);
            result.sandbox = std::move(*sandboxPolicy.value);
            result.approvalsReviewer.value = std::move(reviewer);
            result.cwd.value = std::move(cwd);
            result.model.value = std::move(model);
            result.thread = std::move(*decodedThread);
            result.thread.model = result.model;
            appendDiagnostic(result.diagnostics, approval);
            appendDiagnostic(result.diagnostics, sandboxPolicy);
            if (!result.approvalsReviewer.isKnown()) {
                result.diagnostics.emplace_back(unknownEnumDiagnostic("ApprovalsReviewer", "$.approvalsReviewer"));
            }
            if (!decodeOptionalNullable(value, "serviceTier", result.serviceTier, decodeString, error, context) ||
                !decodeOptionalNullable(
                    value,
                    "reasoningEffort",
                    result.reasoningEffort,
                    [](const Json& input, typed::ReasoningEffort& output) {
                        return decodeString(input, output.value) && !output.value.empty();
                    },
                    error,
                    context)) {
                return std::nullopt;
            }
            if (result.reasoningEffort.hasValue() && !result.reasoningEffort->isKnown()) {
                result.diagnostics.emplace_back(unknownEnumDiagnostic("ReasoningEffort", "$.reasoningEffort"));
            }
            const Json* instructionSources = member(value, "instructionSources");
            if (instructionSources == nullptr) {
                result.instructionSources.clear();
            } else if (!instructionSources->is_array()) {
                fail(error, std::string(context) + " field 'instructionSources' must be an array");
                return std::nullopt;
            } else {
                std::vector<typed::LegacyAppPathString> paths;
                paths.reserve(instructionSources->size());
                for (std::size_t index = 0; index < instructionSources->size(); ++index) {
                    if (!(*instructionSources)[index].is_string()) {
                        fail(error,
                             std::string(context) + " field 'instructionSources[" + std::to_string(index) +
                                 "]' must be a string");
                        return std::nullopt;
                    }
                    paths.emplace_back((*instructionSources)[index].get_ref<const std::string&>());
                }
                result.instructionSources = std::move(paths);
            }
            result.raw = value;
            return result;
        }

        template <typename Response>
        std::optional<Response> decodeThreadWrapperResponse(const Json& value, std::string& error, std::string_view context) {
            if (!value.is_object()) {
                fail(error, std::string(context) + " must be an object");
                return std::nullopt;
            }
            const Json* thread = member(value, "thread");
            if (thread == nullptr) {
                fail(error, std::string(context) + " is missing required field 'thread'");
                return std::nullopt;
            }
            std::optional<typed::Thread> decoded = decodeThread(*thread, error);
            if (!decoded) {
                error = std::string(context) + " field 'thread': " + error;
                return std::nullopt;
            }
            if (std::any_of(decoded->turns.begin(), decoded->turns.end(), hasMalformedKnownPayload)) {
                fail(error, std::string(context) + " field 'thread' contains a malformed known payload");
                return std::nullopt;
            }
            Response result;
            result.thread = std::move(*decoded);
            result.raw = value;
            return result;
        }

    } // namespace

    ConversationDecodeResult<typed::SubAgentSource> decodeSubAgentSource(const Json& value) noexcept {
        constexpr std::string_view Surface = "SubAgentSource";
        try {
            if (value.is_string()) {
                const std::string discriminator = value.get<std::string>();
                const UnionResolution resolution = resolveUnion(Surface, VariantField, discriminator, "$");
                if (resolution.diagnostic) {
                    return {std::nullopt, resolution.diagnostic};
                }
                if (resolution.entry == nullptr) {
                    auto diagnostic = unknownDiscriminatorDiagnostic(std::string(Surface), "$");
                    return {typed::SubAgentSource{
                                typed::UnknownSubAgentSource{discriminator, value, diagnostic}},
                            std::move(diagnostic)};
                }
                if (resolution.descriptor == nullptr ||
                    resolution.descriptor->shape != ConversationUnionCodecShape::ScalarString) {
                    return malformedUnion<typed::SubAgentSource>(Surface, "$");
                }
                typed::SubAgentSourceKind alternative{discriminator, value, {}};
                return {typed::SubAgentSource{std::move(alternative)}, std::nullopt};
            }
            if (!value.is_object() || value.size() != 1) {
                return malformedUnion<typed::SubAgentSource>(Surface, "$");
            }
            const auto iterator = value.begin();
            const std::string discriminator = iterator.key();
            const UnionResolution resolution = resolveUnion(Surface, VariantField, discriminator, "$");
            if (resolution.diagnostic) {
                return {std::nullopt, resolution.diagnostic};
            }
            if (resolution.entry == nullptr) {
                auto diagnostic = unknownDiscriminatorDiagnostic(std::string(Surface), "$");
                return {typed::SubAgentSource{
                            typed::UnknownSubAgentSource{discriminator, value, diagnostic}},
                        std::move(diagnostic)};
            }
            if (resolution.descriptor == nullptr ||
                resolution.descriptor->shape != ConversationUnionCodecShape::ExternallyTaggedObject) {
                return malformedUnion<typed::SubAgentSource>(Surface, "$");
            }
            const auto* target = std::get_if<ConversationUnionTarget>(&resolution.entry->runtimeTarget);
            if (target == nullptr) {
                return malformedUnion<typed::SubAgentSource>(Surface, "$");
            }
            if (*target == ConversationUnionTarget::SubAgentSourceThreadSpawn) {
                if (!iterator.value().is_object()) {
                    return malformedUnion<typed::SubAgentSource>(Surface, "$.thread_spawn");
                }
                typed::ThreadSpawnSubAgentSource alternative;
                const Json& nested = iterator.value();
                std::string parentThreadId;
                std::string nestedError;
                const Json* depth = member(nested, "depth");
                if (!requiredString(nested, "parent_thread_id", parentThreadId, nestedError, Surface)) {
                    return malformedUnion<typed::SubAgentSource>(Surface, "$.thread_spawn.parent_thread_id");
                }
                if (depth == nullptr || !decodeInt32(*depth, alternative.threadSpawn.depth)) {
                    return malformedUnion<typed::SubAgentSource>(
                        Surface, "$.thread_spawn.depth");
                }
                alternative.threadSpawn.parentThreadId.value = std::move(parentThreadId);
                if (!decodeOptionalNullable(
                        nested, "agent_path", alternative.threadSpawn.agentPath,
                        [](const Json& input, typed::AgentPath& output) { return decodeString(input, output.value); },
                        nestedError, Surface)) {
                    return malformedUnion<typed::SubAgentSource>(
                        Surface, "$.thread_spawn.agent_path");
                }
                if (!decodeOptionalNullable(
                        nested,
                        "agent_nickname",
                        alternative.threadSpawn.agentNickname,
                        decodeString,
                        nestedError,
                        Surface)) {
                    return malformedUnion<typed::SubAgentSource>(
                        Surface, "$.thread_spawn.agent_nickname");
                }
                if (!decodeOptionalNullable(
                        nested,
                        "agent_role",
                        alternative.threadSpawn.agentRole,
                        decodeString,
                        nestedError,
                        Surface)) {
                    return malformedUnion<typed::SubAgentSource>(
                        Surface, "$.thread_spawn.agent_role");
                }
                alternative.raw = value;
                return {typed::SubAgentSource{std::move(alternative)}, std::nullopt};
            }
            if (*target == ConversationUnionTarget::SubAgentSourceOther) {
                if (!iterator.value().is_string()) {
                    return malformedUnion<typed::SubAgentSource>(Surface, "$.other");
                }
                return {typed::SubAgentSource{
                            typed::OtherSubAgentSource{iterator.value().get<std::string>(), value, {}}},
                        std::nullopt};
            }
            return malformedUnion<typed::SubAgentSource>(Surface, "$");
        } catch (...) {
            return malformedUnion<typed::SubAgentSource>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::SessionSource> decodeSessionSource(const Json& value) noexcept {
        constexpr std::string_view Surface = "SessionSource";
        try {
            if (value.is_string()) {
                const std::string discriminator = value.get<std::string>();
                const UnionResolution resolution = resolveUnion(Surface, VariantField, discriminator, "$");
                if (resolution.diagnostic) {
                    return {std::nullopt, resolution.diagnostic};
                }
                if (resolution.entry == nullptr) {
                    auto diagnostic = unknownDiscriminatorDiagnostic(std::string(Surface), "$");
                    return {typed::SessionSource{
                                typed::UnknownSessionSourceAlternative{discriminator, value, diagnostic}},
                            std::move(diagnostic)};
                }
                if (resolution.descriptor == nullptr ||
                    resolution.descriptor->shape != ConversationUnionCodecShape::ScalarString) {
                    return malformedUnion<typed::SessionSource>(Surface, "$");
                }
                typed::SessionSourceKind alternative{discriminator, value, {}};
                return {typed::SessionSource{std::move(alternative)}, std::nullopt};
            }
            if (!value.is_object() || value.size() != 1) {
                return malformedUnion<typed::SessionSource>(Surface, "$");
            }
            const auto iterator = value.begin();
            const std::string discriminator = iterator.key();
            const UnionResolution resolution = resolveUnion(Surface, VariantField, discriminator, "$");
            if (resolution.diagnostic) {
                return {std::nullopt, resolution.diagnostic};
            }
            if (resolution.entry == nullptr) {
                auto diagnostic = unknownDiscriminatorDiagnostic(std::string(Surface), "$");
                return {typed::SessionSource{
                            typed::UnknownSessionSourceAlternative{discriminator, value, diagnostic}},
                        std::move(diagnostic)};
            }
            if (resolution.descriptor == nullptr ||
                resolution.descriptor->shape != ConversationUnionCodecShape::ExternallyTaggedObject) {
                return malformedUnion<typed::SessionSource>(Surface, "$");
            }
            const auto* target = std::get_if<ConversationUnionTarget>(&resolution.entry->runtimeTarget);
            if (target == nullptr) {
                return malformedUnion<typed::SessionSource>(Surface, "$");
            }
            if (*target == ConversationUnionTarget::SessionSourceCustom) {
                if (!iterator.value().is_string()) {
                    return malformedUnion<typed::SessionSource>(Surface, "$.custom");
                }
                return {typed::SessionSource{
                            typed::CustomSessionSource{iterator.value().get<std::string>(), value, {}}},
                        std::nullopt};
            }
            if (*target == ConversationUnionTarget::SessionSourceSubAgent) {
                auto decoded = decodeSubAgentSource(iterator.value());
                if (!decoded.value) {
                    if (decoded.diagnostic) {
                        typed::DecodeDiagnostic nestedDiagnostic =
                            *decoded.diagnostic;
                        if (nestedDiagnostic.fieldPath.starts_with("$")) {
                            nestedDiagnostic.fieldPath =
                                "$.subAgent" +
                                nestedDiagnostic.fieldPath.substr(1);
                        }
                        return {std::nullopt,
                                std::move(nestedDiagnostic)};
                    }
                    auto diagnostic = malformedDiagnostic(
                        std::string(Surface), "$.subAgent");
                    return {std::nullopt, std::move(diagnostic)};
                }
                typed::SubAgentSessionSource alternative{std::move(*decoded.value), value, {}};
                if (decoded.diagnostic) {
                    typed::DecodeDiagnostic nestedDiagnostic = *decoded.diagnostic;
                    if (nestedDiagnostic.fieldPath.starts_with("$")) {
                        nestedDiagnostic.fieldPath =
                            "$.subAgent" + nestedDiagnostic.fieldPath.substr(1);
                    }
                    alternative.diagnostics.emplace_back(nestedDiagnostic);
                    return {typed::SessionSource{std::move(alternative)},
                            std::move(nestedDiagnostic)};
                }
                return {typed::SessionSource{std::move(alternative)}, std::nullopt};
            }
            return malformedUnion<typed::SessionSource>(Surface, "$");
        } catch (...) {
            return malformedUnion<typed::SessionSource>(Surface, "$");
        }
    }

    ConversationDecodeResult<typed::ThreadStatus> decodeThreadStatus(const Json& value) noexcept {
        constexpr std::string_view Surface = "ThreadStatus";
        try {
            if (!value.is_object()) {
                return malformedUnion<typed::ThreadStatus>(Surface, "$");
            }
            const Json* type = member(value, "type");
            if (type == nullptr || !type->is_string()) {
                return malformedUnion<typed::ThreadStatus>(Surface, "$.type");
            }
            const std::string discriminator = type->get<std::string>();
            const UnionResolution resolution = resolveUnion(Surface, TypeField, discriminator, "$.type");
            if (resolution.diagnostic) {
                return {std::nullopt, resolution.diagnostic};
            }
            if (resolution.entry == nullptr) {
                auto diagnostic = unknownDiscriminatorDiagnostic(std::string(Surface), "$.type");
                return {typed::ThreadStatus{
                            typed::UnknownThreadStatus{discriminator, value, diagnostic}},
                        std::move(diagnostic)};
            }
            if (resolution.descriptor == nullptr ||
                resolution.descriptor->shape != ConversationUnionCodecShape::InternallyTaggedObject) {
                return malformedUnion<typed::ThreadStatus>(Surface, "$.type");
            }
            const auto* target = std::get_if<ConversationUnionTarget>(&resolution.entry->runtimeTarget);
            if (target == nullptr) {
                return malformedUnion<typed::ThreadStatus>(Surface, "$.type");
            }
            if (*target == ConversationUnionTarget::ThreadStatusNotLoaded) {
                return {typed::ThreadStatus{typed::NotLoadedThreadStatus{value, {}}}, std::nullopt};
            }
            if (*target == ConversationUnionTarget::ThreadStatusIdle) {
                return {typed::ThreadStatus{typed::IdleThreadStatus{value, {}}}, std::nullopt};
            }
            if (*target == ConversationUnionTarget::ThreadStatusSystemError) {
                return {typed::ThreadStatus{typed::SystemErrorThreadStatus{value, {}}}, std::nullopt};
            }
            if (*target == ConversationUnionTarget::ThreadStatusActive) {
                const Json* activeFlags = member(value, "activeFlags");
                if (activeFlags == nullptr || !activeFlags->is_array()) {
                    return malformedUnion<typed::ThreadStatus>(Surface, "$.activeFlags");
                }
                typed::ActiveThreadStatus alternative;
                alternative.raw = value;
                std::optional<typed::DecodeDiagnostic> firstDiagnostic;
                alternative.activeFlags.reserve(activeFlags->size());
                for (std::size_t index = 0; index < activeFlags->size(); ++index) {
                    if (!(*activeFlags)[index].is_string()) {
                        return malformedUnion<typed::ThreadStatus>(
                            Surface, "$.activeFlags[" + std::to_string(index) + "]");
                    }
                    typed::ThreadActiveFlag flag{(*activeFlags)[index].get<std::string>()};
                    if (!flag.isKnown()) {
                        auto diagnostic = unknownEnumDiagnostic(
                            "ThreadActiveFlag", "$.activeFlags[" + std::to_string(index) + "]");
                        if (!firstDiagnostic) {
                            firstDiagnostic = diagnostic;
                        }
                        alternative.diagnostics.emplace_back(std::move(diagnostic));
                    }
                    alternative.activeFlags.emplace_back(std::move(flag));
                }
                return {typed::ThreadStatus{std::move(alternative)}, std::move(firstDiagnostic)};
            }
            return malformedUnion<typed::ThreadStatus>(Surface, "$.type");
        } catch (...) {
            return malformedUnion<typed::ThreadStatus>(Surface, "$");
        }
    }

    std::optional<Json> encodeThreadArchiveParams(const typed::ThreadArchiveParams& params, std::string& error) {
        return encodeThreadIdOnly(params, error, "thread/archive");
    }

    std::optional<Json> encodeThreadCompactStartParams(const typed::ThreadCompactStartParams& params, std::string& error) {
        return encodeThreadIdOnly(params, error, "thread/compact/start");
    }

    std::optional<Json> encodeThreadDeleteParams(const typed::ThreadDeleteParams& params, std::string& error) {
        return encodeThreadIdOnly(params, error, "thread/delete");
    }

    std::optional<Json> encodeThreadForkParams(const typed::ThreadForkParams& params, std::string& error) {
        try {
            error.clear();
            Json result{{"threadId", params.threadId.value}};
            if (!encodeThreadOverrides(result, params, error) ||
                !encodeOptionalNullable(result, "lastTurnId", params.lastTurnId, openValue<typed::TurnId>, error) ||
                !encodeOptionalNullable(result, "threadSource", params.threadSource, encodeThreadSourceValue, error)) {
                return std::nullopt;
            }
            if (params.ephemeral) {
                result["ephemeral"] = *params.ephemeral;
            }
            return std::optional<Json>{std::in_place, std::move(result)};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/fork parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/fork parameters";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeThreadGoalClearParams(const typed::ThreadGoalClearParams& params, std::string& error) {
        return encodeThreadIdOnly(params, error, "thread/goal/clear");
    }

    std::optional<Json> encodeThreadGoalGetParams(const typed::ThreadGoalGetParams& params, std::string& error) {
        return encodeThreadIdOnly(params, error, "thread/goal/get");
    }

    std::optional<Json> encodeThreadGoalSetParams(const typed::ThreadGoalSetParams& params, std::string& error) {
        try {
            error.clear();
            Json result{{"threadId", params.threadId.value}};
            if (!encodeOptionalNullable(result, "objective", params.objective, scalarValue<std::string>, error) ||
                !encodeOptionalNullable(result, "status", params.status, openValue<typed::ThreadGoalStatus>, error) ||
                !encodeOptionalNullable(result, "tokenBudget", params.tokenBudget, scalarValue<std::int64_t>, error)) {
                return std::nullopt;
            }
            return std::optional<Json>{std::in_place, std::move(result)};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/goal/set parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/goal/set parameters";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeThreadInjectItemsParams(const typed::ThreadInjectItemsParams& params, std::string& error) {
        try {
            error.clear();
            return std::optional<Json>{
                std::in_place, Json{{"threadId", params.threadId.value}, {"items", params.items}}};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/inject_items parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/inject_items parameters";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeThreadListParams(const typed::ThreadListParams& params, std::string& error) {
        try {
            error.clear();
            Json result = Json::object();
            auto encodeSourceKinds = [](const std::vector<typed::ThreadSourceKind>& values, std::string&) -> std::optional<Json> {
                Json array = Json::array();
                for (const auto& value : values) {
                    array.emplace_back(value.value);
                }
                return std::optional<Json>{std::in_place, std::move(array)};
            };
            auto encodeStrings = [](const std::vector<std::string>& values, std::string&) -> std::optional<Json> {
                return std::optional<Json>{std::in_place, values};
            };
            auto encodeCwd = [](const typed::ThreadListCwdFilter& value, std::string&) -> std::optional<Json> {
                return std::optional<Json>{
                    std::in_place,
                    std::visit([](const auto& alternative) -> Json { return alternative; }, value.value)};
            };
            if (!encodeOptionalNullable(result, "sourceKinds", params.sourceKinds, encodeSourceKinds, error) ||
                !encodeOptionalNullable(result, "archived", params.archived, scalarValue<bool>, error) ||
                !encodeOptionalNullable(result, "cursor", params.cursor, scalarValue<std::string>, error) ||
                !encodeOptionalNullable(result, "cwd", params.cwd, encodeCwd, error) ||
                !encodeOptionalNullable(result, "limit", params.limit, scalarValue<std::uint32_t>, error) ||
                !encodeOptionalNullable(result, "modelProviders", params.modelProviders, encodeStrings, error) ||
                !encodeOptionalNullable(result, "searchTerm", params.searchTerm, scalarValue<std::string>, error) ||
                !encodeOptionalNullable(result, "sortDirection", params.sortDirection, openValue<typed::SortDirection>, error) ||
                !encodeOptionalNullable(result, "sortKey", params.sortKey, openValue<typed::ThreadSortKey>, error)) {
                return std::nullopt;
            }
            if (params.useStateDbOnly) {
                result["useStateDbOnly"] = *params.useStateDbOnly;
            }
            return std::optional<Json>{std::in_place, std::move(result)};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/list parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/list parameters";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeThreadLoadedListParams(const typed::ThreadLoadedListParams& params, std::string& error) {
        try {
            error.clear();
            Json result = Json::object();
            if (!encodeOptionalNullable(result, "cursor", params.cursor, scalarValue<std::string>, error) ||
                !encodeOptionalNullable(result, "limit", params.limit, scalarValue<std::uint32_t>, error)) {
                return std::nullopt;
            }
            return std::optional<Json>{std::in_place, std::move(result)};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/loaded/list parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/loaded/list parameters";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeThreadMetadataUpdateParams(const typed::ThreadMetadataUpdateParams& params, std::string& error) {
        try {
            error.clear();
            Json result{{"threadId", params.threadId.value}};
            auto encodeGitInfo = [](const typed::ThreadMetadataGitInfoUpdateParams& value,
                                    std::string& nestedError) -> std::optional<Json> {
                Json nested = Json::object();
                if (!encodeOptionalNullable(nested, "branch", value.branch, scalarValue<std::string>, nestedError) ||
                    !encodeOptionalNullable(nested, "originUrl", value.originUrl, scalarValue<std::string>, nestedError) ||
                    !encodeOptionalNullable(nested, "sha", value.sha, scalarValue<std::string>, nestedError)) {
                    return std::nullopt;
                }
                return std::optional<Json>{std::in_place, std::move(nested)};
            };
            if (!encodeOptionalNullable(result, "gitInfo", params.gitInfo, encodeGitInfo, error)) {
                return std::nullopt;
            }
            return std::optional<Json>{std::in_place, std::move(result)};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/metadata/update parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/metadata/update parameters";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeThreadReadParams(const typed::ThreadReadParams& params, std::string& error) {
        try {
            error.clear();
            Json result{{"threadId", params.threadId.value}};
            if (params.includeTurns) {
                result["includeTurns"] = *params.includeTurns;
            }
            return std::optional<Json>{std::in_place, std::move(result)};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/read parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/read parameters";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeThreadResumeParams(const typed::ThreadResumeParams& params, std::string& error) {
        try {
            error.clear();
            Json result{{"threadId", params.threadId.value}};
            if (!encodeThreadOverrides(result, params, error) ||
                !encodeOptionalNullable(result, "personality", params.personality, openValue<typed::Personality>, error)) {
                return std::nullopt;
            }
            return std::optional<Json>{std::in_place, std::move(result)};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/resume parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/resume parameters";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeThreadRollbackParams(const typed::ThreadRollbackParams& params, std::string& error) {
        try {
            error.clear();
            return std::optional<Json>{
                std::in_place, Json{{"threadId", params.threadId.value}, {"numTurns", params.numTurns}}};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/rollback parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/rollback parameters";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeThreadSetNameParams(const typed::ThreadSetNameParams& params, std::string& error) {
        try {
            error.clear();
            return std::optional<Json>{
                std::in_place, Json{{"threadId", params.threadId.value}, {"name", params.name}}};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/name/set parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/name/set parameters";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeThreadShellCommandParams(const typed::ThreadShellCommandParams& params, std::string& error) {
        try {
            error.clear();
            return std::optional<Json>{
                std::in_place, Json{{"threadId", params.threadId.value}, {"command", params.command}}};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/shellCommand parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/shellCommand parameters";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeThreadStartParams(const typed::ThreadStartParams& params, std::string& error) {
        try {
            error.clear();
            Json result = Json::object();
            if (!encodeThreadOverrides(result, params, error) ||
                !encodeOptionalNullable(
                    result, "sessionStartSource", params.sessionStartSource, openValue<typed::ThreadStartSource>, error) ||
                !encodeOptionalNullable(result, "serviceName", params.serviceName, scalarValue<std::string>, error) ||
                !encodeOptionalNullable(result, "personality", params.personality, openValue<typed::Personality>, error) ||
                !encodeOptionalNullable(result, "ephemeral", params.ephemeral, scalarValue<bool>, error) ||
                !encodeOptionalNullable(result, "threadSource", params.threadSource, encodeThreadSourceValue, error)) {
                return std::nullopt;
            }
            return std::optional<Json>{std::in_place, std::move(result)};
        } catch (const std::exception& exception) {
            error = std::string("failed to encode thread/start parameters: ") + exception.what();
        } catch (...) {
            error = "failed to encode thread/start parameters";
        }
        return std::nullopt;
    }

    std::optional<Json> encodeThreadUnarchiveParams(const typed::ThreadUnarchiveParams& params, std::string& error) {
        return encodeThreadIdOnly(params, error, "thread/unarchive");
    }

    std::optional<Json> encodeThreadUnsubscribeParams(const typed::ThreadUnsubscribeParams& params, std::string& error) {
        return encodeThreadIdOnly(params, error, "thread/unsubscribe");
    }

    std::optional<typed::Unit> decodeUnitResult(const Json& value, std::string& error) {
        error.clear();
        if (!value.is_object() || !value.empty()) {
            fail(error, "Unit successful result must be the exact empty object");
            return std::nullopt;
        }
        return typed::Unit{};
    }

    std::optional<typed::Unit> decodeThreadArchiveResult(const Json& value, std::string& error) {
        return decodeUnitResult(value, error);
    }

    std::optional<typed::Unit> decodeThreadCompactStartResult(const Json& value, std::string& error) {
        return decodeUnitResult(value, error);
    }

    std::optional<typed::Unit> decodeThreadDeleteResult(const Json& value, std::string& error) {
        return decodeUnitResult(value, error);
    }

    std::optional<typed::Unit> decodeThreadInjectItemsResult(const Json& value, std::string& error) {
        return decodeUnitResult(value, error);
    }

    std::optional<typed::Unit> decodeThreadSetNameResult(const Json& value, std::string& error) {
        return decodeUnitResult(value, error);
    }

    std::optional<typed::Unit> decodeThreadShellCommandResult(const Json& value, std::string& error) {
        return decodeUnitResult(value, error);
    }

    std::optional<typed::Thread> decodeThread(const Json& value, std::string& error) {
        try {
            error.clear();
            if (!value.is_object()) {
                fail(error, "Thread must be an object");
                return std::nullopt;
            }
            typed::Thread result;
            std::string id;
            std::string cwd;
            const Json* source = member(value, "source");
            const Json* status = member(value, "status");
            const Json* turns = member(value, "turns");
            if (source == nullptr || status == nullptr || turns == nullptr || !turns->is_array() ||
                !requiredString(value, "id", id, error, "Thread") ||
                !requiredString(value, "preview", result.preview, error, "Thread") ||
                !requiredBool(value, "ephemeral", result.ephemeral, error, "Thread") ||
                !requiredString(value, "modelProvider", result.modelProvider, error, "Thread") ||
                !requiredInt64(value, "createdAt", result.createdAt, error, "Thread") ||
                !requiredInt64(value, "updatedAt", result.updatedAt, error, "Thread") ||
                !requiredString(value, "cwd", cwd, error, "Thread") ||
                !requiredString(value, "cliVersion", result.cliVersion, error, "Thread") ||
                !requiredString(value, "sessionId", result.sessionId.value, error, "Thread")) {
                if (error.empty()) {
                    fail(error, "Thread is missing or has invalid required source/status/turns fields");
                }
                return std::nullopt;
            }
            auto decodedSource = decodeSessionSource(*source);
            auto decodedStatus = decodeThreadStatus(*status);
            if (!decodedSource.value) {
                fail(error, "Thread field 'source' is malformed");
                return std::nullopt;
            }
            if (!decodedStatus.value) {
                fail(error, "Thread field 'status' is malformed");
                return std::nullopt;
            }
            result.id.value = std::move(id);
            result.cwd.value = std::move(cwd);
            result.source = std::move(*decodedSource.value);
            result.status = std::move(*decodedStatus.value);
            appendDiagnostic(result.diagnostics, decodedSource);
            appendDiagnostic(result.diagnostics, decodedStatus);
            if (!decodeOptionalNullable(value, "path", result.path, decodeString, error, "Thread") ||
                !decodeOptionalNullable(value, "agentRole", result.agentRole, decodeString, error, "Thread") ||
                !decodeOptionalNullable(value, "agentNickname", result.agentNickname, decodeString, error, "Thread") ||
                !decodeOptionalNullable(value, "name", result.name, decodeString, error, "Thread") ||
                !decodeOptionalNullable(
                    value,
                    "gitInfo",
                    result.gitInfo,
                    [&](const Json& input, typed::GitInfo& output) {
                        std::string nestedError;
                        return decodeGitInfoValue(input, output, nestedError);
                    },
                    error,
                    "Thread") ||
                !decodeOptionalNullable(value, "forkedFromId", result.forkedFromId, decodeThreadId, error, "Thread") ||
                !decodeOptionalNullable(value, "parentThreadId", result.parentThreadId, decodeThreadId, error, "Thread") ||
                !decodeOptionalNullable(value, "recencyAt", result.recencyAt, decodeInt64, error, "Thread") ||
                !decodeOptionalNullable(value, "threadSource", result.threadSource, decodeThreadSource, error, "Thread")) {
                return std::nullopt;
            }
            if (result.name.hasValue()) {
                result.title = *result.name;
            } else {
                result.title.reset();
            }
            result.turns.reserve(turns->size());
            for (std::size_t index = 0; index < turns->size(); ++index) {
                std::string turnError;
                auto turn = decodeTurn((*turns)[index], result.id, turnError);
                if (!turn) {
                    fail(error, "Thread field 'turns[" + std::to_string(index) + "]': " + turnError);
                    return std::nullopt;
                }
                result.turns.emplace_back(std::move(*turn));
            }
            result.raw = value;
            return result;
        } catch (const std::exception& exception) {
            error = std::string("failed to decode Thread: ") + exception.what();
        } catch (...) {
            error = "failed to decode Thread";
        }
        return std::nullopt;
    }

    std::optional<typed::Thread>
    decodeThread(const Json& value, std::string& error, std::optional<typed::ModelId> contextualModel) {
        std::optional<typed::Thread> result = decodeThread(value, error);
        if (result) {
            result->model = std::move(contextualModel);
        }
        return result;
    }

    std::optional<typed::ThreadForkResponse> decodeThreadForkResponse(const Json& value, std::string& error) {
        return decodeThreadLaunchResponse<typed::ThreadForkResponse>(value, error, "ThreadForkResponse");
    }

    std::optional<typed::ThreadGoalClearResponse> decodeThreadGoalClearResponse(const Json& value, std::string& error) {
        error.clear();
        if (!value.is_object()) {
            fail(error, "ThreadGoalClearResponse must be an object");
            return std::nullopt;
        }
        typed::ThreadGoalClearResponse result;
        if (!requiredBool(value, "cleared", result.cleared, error, "ThreadGoalClearResponse")) {
            return std::nullopt;
        }
        result.raw = value;
        return result;
    }

    std::optional<typed::ThreadGoalGetResponse> decodeThreadGoalGetResponse(const Json& value, std::string& error) {
        error.clear();
        if (!value.is_object()) {
            fail(error, "ThreadGoalGetResponse must be an object");
            return std::nullopt;
        }
        typed::ThreadGoalGetResponse result;
        if (!decodeOptionalNullable(
                value,
                "goal",
                result.goal,
                [&](const Json& input, typed::ThreadGoal& output) {
                    std::string nestedError;
                    auto decoded = decodeThreadGoal(input, nestedError);
                    if (!decoded) {
                        return false;
                    }
                    output = std::move(*decoded);
                    return true;
                },
                error,
                "ThreadGoalGetResponse")) {
            return std::nullopt;
        }
        result.raw = value;
        return result;
    }

    std::optional<typed::ThreadGoalSetResponse> decodeThreadGoalSetResponse(const Json& value, std::string& error) {
        error.clear();
        if (!value.is_object()) {
            fail(error, "ThreadGoalSetResponse must be an object");
            return std::nullopt;
        }
        const Json* goal = member(value, "goal");
        if (goal == nullptr) {
            fail(error, "ThreadGoalSetResponse is missing required field 'goal'");
            return std::nullopt;
        }
        auto decoded = decodeThreadGoal(*goal, error);
        if (!decoded) {
            error = "ThreadGoalSetResponse field 'goal': " + error;
            return std::nullopt;
        }
        typed::ThreadGoalSetResponse result;
        result.goal = std::move(*decoded);
        result.raw = value;
        return result;
    }

    std::optional<typed::ThreadListResponse> decodeThreadListResponse(const Json& value, std::string& error) {
        error.clear();
        if (!value.is_object()) {
            fail(error, "ThreadListResponse must be an object");
            return std::nullopt;
        }
        const Json* data = member(value, "data");
        if (data == nullptr || !data->is_array()) {
            fail(error, "ThreadListResponse field 'data' must be an array");
            return std::nullopt;
        }
        typed::ThreadListResponse result;
        if (!decodeOptionalNullable(value, "nextCursor", result.nextCursor, decodeString, error, "ThreadListResponse") ||
            !decodeOptionalNullable(
                value, "backwardsCursor", result.backwardsCursor, decodeString, error, "ThreadListResponse")) {
            return std::nullopt;
        }
        result.data.reserve(data->size());
        for (std::size_t index = 0; index < data->size(); ++index) {
            std::string nestedError;
            auto thread = decodeThread((*data)[index], nestedError);
            if (!thread) {
                fail(error, "ThreadListResponse field 'data[" + std::to_string(index) + "]': " + nestedError);
                return std::nullopt;
            }
            if (std::any_of(thread->turns.begin(), thread->turns.end(), hasMalformedKnownPayload)) {
                fail(error,
                     "ThreadListResponse field 'data[" + std::to_string(index) +
                         "]' contains a malformed known payload");
                return std::nullopt;
            }
            result.data.emplace_back(std::move(*thread));
        }
        result.raw = value;
        return result;
    }

    std::optional<typed::ThreadLoadedListResponse> decodeThreadLoadedListResponse(const Json& value, std::string& error) {
        error.clear();
        if (!value.is_object()) {
            fail(error, "ThreadLoadedListResponse must be an object");
            return std::nullopt;
        }
        const Json* data = member(value, "data");
        if (data == nullptr || !data->is_array()) {
            fail(error, "ThreadLoadedListResponse field 'data' must be an array");
            return std::nullopt;
        }
        typed::ThreadLoadedListResponse result;
        result.data.reserve(data->size());
        for (std::size_t index = 0; index < data->size(); ++index) {
            if (!(*data)[index].is_string()) {
                fail(error, "ThreadLoadedListResponse field 'data[" + std::to_string(index) + "]' must be a string");
                return std::nullopt;
            }
            result.data.emplace_back(typed::ThreadId{(*data)[index].get<std::string>()});
        }
        if (!decodeOptionalNullable(
                value, "nextCursor", result.nextCursor, decodeString, error, "ThreadLoadedListResponse")) {
            return std::nullopt;
        }
        result.raw = value;
        return result;
    }

    std::optional<typed::ThreadMetadataUpdateResponse>
    decodeThreadMetadataUpdateResponse(const Json& value, std::string& error) {
        return decodeThreadWrapperResponse<typed::ThreadMetadataUpdateResponse>(
            value, error, "ThreadMetadataUpdateResponse");
    }

    std::optional<typed::ThreadReadResponse> decodeThreadReadResponse(const Json& value, std::string& error) {
        return decodeThreadWrapperResponse<typed::ThreadReadResponse>(value, error, "ThreadReadResponse");
    }

    std::optional<typed::ThreadResumeResponse> decodeThreadResumeResponse(const Json& value, std::string& error) {
        return decodeThreadLaunchResponse<typed::ThreadResumeResponse>(value, error, "ThreadResumeResponse");
    }

    std::optional<typed::ThreadRollbackResponse> decodeThreadRollbackResponse(const Json& value, std::string& error) {
        return decodeThreadWrapperResponse<typed::ThreadRollbackResponse>(value, error, "ThreadRollbackResponse");
    }

    std::optional<typed::ThreadStartResponse> decodeThreadStartResponse(const Json& value, std::string& error) {
        return decodeThreadLaunchResponse<typed::ThreadStartResponse>(value, error, "ThreadStartResponse");
    }

    std::optional<typed::ThreadUnarchiveResponse> decodeThreadUnarchiveResponse(const Json& value, std::string& error) {
        return decodeThreadWrapperResponse<typed::ThreadUnarchiveResponse>(value, error, "ThreadUnarchiveResponse");
    }

    std::optional<typed::ThreadUnsubscribeResponse> decodeThreadUnsubscribeResponse(const Json& value, std::string& error) {
        error.clear();
        if (!value.is_object()) {
            fail(error, "ThreadUnsubscribeResponse must be an object");
            return std::nullopt;
        }
        typed::ThreadUnsubscribeResponse result;
        if (!requiredString(value, "status", result.status.value, error, "ThreadUnsubscribeResponse")) {
            return std::nullopt;
        }
        if (!result.status.isKnown()) {
            result.diagnostics.emplace_back(unknownEnumDiagnostic("ThreadUnsubscribeStatus", "$.status"));
        }
        result.raw = value;
        return result;
    }

    std::optional<Json> encodeThreadStartParams(const typed::ThreadStartOptions& options, std::string& error) {
        return encodeThreadStartParams(typed::toThreadStartParams(options), error);
    }

    std::optional<Json>
    encodeThreadResumeParams(const typed::ThreadId& threadId, const typed::ThreadResumeOptions& options, std::string& error) {
        return encodeThreadResumeParams(typed::toThreadResumeParams(threadId, options), error);
    }

    std::optional<Json> encodeThreadListParams(const typed::ThreadListOptions& options, std::string& error) {
        return encodeThreadListParams(typed::toThreadListParams(options), error);
    }

    std::optional<Json>
    encodeThreadReadParams(const typed::ThreadId& threadId, const typed::ThreadReadOptions& options, std::string& error) {
        return encodeThreadReadParams(typed::toThreadReadParams(threadId, options), error);
    }

    std::optional<typed::Thread> decodeThreadOperationResult(const Json& value, std::string& error) {
        auto response = decodeThreadStartResponse(value, error);
        if (!response) {
            return std::nullopt;
        }
        return std::move(response->thread);
    }

    std::optional<typed::ThreadPage> decodeThreadListResult(const Json& value, std::string& error) {
        return decodeThreadListResponse(value, error);
    }

    std::optional<typed::Thread> decodeThreadReadResult(const Json& value, std::string& error) {
        auto response = decodeThreadReadResponse(value, error);
        if (!response) {
            return std::nullopt;
        }
        return std::move(response->thread);
    }

} // namespace ai::openai::codex::detail
