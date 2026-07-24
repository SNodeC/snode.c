/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/detail/ThreadCodec.h"
#include "support/TestResult.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#ifndef CODEX_A1_FIXTURE_ROOT
#error "CODEX_A1_FIXTURE_ROOT must name the checked App Server fixture corpus"
#endif

#ifndef CODEX_A1_OPERATION_PRODUCTION_COVERAGE
#error "CODEX_A1_OPERATION_PRODUCTION_COVERAGE must name the checked production-coverage evidence"
#endif

namespace {
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    codex::Json readJson(const std::filesystem::path& path) {
        std::ifstream input(path);
        if (!input) {
            throw std::runtime_error("unable to open " + path.string());
        }
        return codex::Json::parse(input);
    }

    std::vector<std::string> strings(const codex::Json& value) {
        std::vector<std::string> result;
        for (const codex::Json& element : value) {
            result.emplace_back(element.get<std::string>());
        }
        return result;
    }

    std::string_view targetToken(detail::ConversationUnionTarget target) {
        using enum detail::ConversationUnionTarget;
        switch (target) {
            case SessionSourceAppServer:
                return "ConversationUnionTarget::SessionSourceAppServer";
            case SessionSourceCli:
                return "ConversationUnionTarget::SessionSourceCli";
            case SessionSourceCustom:
                return "ConversationUnionTarget::SessionSourceCustom";
            case SessionSourceExec:
                return "ConversationUnionTarget::SessionSourceExec";
            case SessionSourceSubAgent:
                return "ConversationUnionTarget::SessionSourceSubAgent";
            case SessionSourceUnknown:
                return "ConversationUnionTarget::SessionSourceUnknown";
            case SessionSourceVscode:
                return "ConversationUnionTarget::SessionSourceVscode";
            case SubAgentSourceCompact:
                return "ConversationUnionTarget::SubAgentSourceCompact";
            case SubAgentSourceMemoryConsolidation:
                return "ConversationUnionTarget::SubAgentSourceMemoryConsolidation";
            case SubAgentSourceOther:
                return "ConversationUnionTarget::SubAgentSourceOther";
            case SubAgentSourceReview:
                return "ConversationUnionTarget::SubAgentSourceReview";
            case SubAgentSourceThreadSpawn:
                return "ConversationUnionTarget::SubAgentSourceThreadSpawn";
            case ThreadStatusActive:
                return "ConversationUnionTarget::ThreadStatusActive";
            case ThreadStatusIdle:
                return "ConversationUnionTarget::ThreadStatusIdle";
            case ThreadStatusNotLoaded:
                return "ConversationUnionTarget::ThreadStatusNotLoaded";
            case ThreadStatusSystemError:
                return "ConversationUnionTarget::ThreadStatusSystemError";
            default:
                return "ConversationUnionTarget::Count";
        }
    }

    std::string_view shapeToken(detail::ConversationUnionCodecShape shape) {
        using enum detail::ConversationUnionCodecShape;
        switch (shape) {
            case ScalarString:
                return "ScalarString";
            case ExternallyTaggedObject:
                return "ExternallyTaggedObject";
            case InternallyTaggedObject:
                return "InternallyTaggedObject";
            case Count:
                return "Count";
        }
        return "Count";
    }

    std::string_view directionToken(detail::ConversationUnionCodecDirection direction) {
        using enum detail::ConversationUnionCodecDirection;
        switch (direction) {
            case DecodeOnly:
                return "DecodeOnly";
            case Bidirectional:
                return "Bidirectional";
            case Count:
                return "Count";
        }
        return "Count";
    }

    std::string diagnosticCode(typed::DecodeIssueKind kind) {
        using enum typed::DecodeIssueKind;
        switch (kind) {
            case UnknownMethod:
                return "UnknownMethod";
            case UnknownDiscriminator:
                return "UnknownDiscriminator";
            case UnknownEnumValue:
                return "UnknownEnumValue";
            case MalformedKnownPayload:
                return "MalformedKnownPayload";
        }
        return "MalformedKnownPayload";
    }

    template <typename Value>
    std::vector<std::string> intrinsicCodes(const detail::ConversationDecodeResult<Value>& decoded) {
        std::vector<std::string> result;
        if (decoded.value) {
            result.emplace_back("Decoded");
        }
        if (decoded.diagnostic) {
            result.emplace_back(diagnosticCode(decoded.diagnostic->kind));
        }
        if (result.empty()) {
            result.emplace_back("DecoderFailureWithoutDiagnostic");
        }
        return result;
    }

    bool diagnosticMatches(const std::optional<typed::DecodeDiagnostic>& diagnostic,
                           std::string_view expectedSurface,
                           const codex::Json& expectedCodes,
                           const codex::Json& expectedPath) {
        const std::vector<std::string> codes = strings(expectedCodes);
        if (codes == std::vector<std::string>{"Decoded"}) {
            return !diagnostic && expectedPath.is_null();
        }
        if (!diagnostic || expectedPath.is_null() || !expectedPath.is_string()) {
            return false;
        }
        const std::string& code = codes.back();
        const bool expectedForwardCompatibility = code == "UnknownDiscriminator" || code == "UnknownEnumValue";
        return diagnosticCode(diagnostic->kind) == code && diagnostic->surface == expectedSurface &&
               diagnostic->fieldPath == expectedPath.get<std::string>() &&
               diagnostic->severity == (expectedForwardCompatibility ? typed::DecodeIssueSeverity::ForwardCompatibility
                                                                     : typed::DecodeIssueSeverity::ProtocolWarning);
    }

    void collectPayloadStrings(const codex::Json& value, std::vector<std::string>& result) {
        if (value.is_string()) {
            const std::string payload = value.get<std::string>();
            if (!payload.empty()) {
                result.emplace_back(payload);
            }
            return;
        }
        if (value.is_array()) {
            for (const codex::Json& element : value) {
                collectPayloadStrings(element, result);
            }
            return;
        }
        if (value.is_object()) {
            for (const auto& [_, member] : value.items()) {
                collectPayloadStrings(member, result);
            }
        }
    }

    bool diagnosticOmitsPayloadValues(const std::optional<typed::DecodeDiagnostic>& diagnostic, const codex::Json& raw) {
        if (!diagnostic) {
            return true;
        }
        std::vector<std::string> payloads;
        collectPayloadStrings(raw, payloads);
        return std::none_of(payloads.begin(), payloads.end(), [&](const std::string& payload) {
            return diagnostic->message.find(payload) != std::string::npos;
        });
    }

    template <typename Union>
    bool rawMatches(const std::optional<Union>& decoded, const codex::Json& raw) {
        return decoded && std::visit(
                              [&](const auto& alternative) {
                                  return alternative.raw == raw;
                              },
                              *decoded);
    }

    template <typename Value, typename ToString>
    bool nullableStringMatches(const typed::OptionalNullable<Value>& decoded,
                               const codex::Json& object,
                               std::string_view field,
                               ToString&& toString) {
        const auto iterator = object.find(field);
        if (iterator == object.end()) {
            return decoded.isOmitted();
        }
        if (iterator->is_null()) {
            return decoded.isNull();
        }
        return iterator->is_string() && decoded.hasValue() && toString(*decoded.value) == iterator->get<std::string>();
    }

    bool subAgentFieldsMatch(const detail::ConversationDecodeResult<typed::SubAgentSource>& decoded,
                             const codex::Json& raw,
                             const std::optional<detail::ConversationUnionTarget>& target) {
        if (!decoded.value || !rawMatches(decoded.value, raw)) {
            return false;
        }
        if (!target) {
            if (raw.is_string()) {
                if (!decoded.diagnostic) {
                    const auto* value = std::get_if<typed::SubAgentSourceKind>(&*decoded.value);
                    return value && value->value == raw.get<std::string>() && value->diagnostics.empty();
                }
                const auto* value = std::get_if<typed::UnknownSubAgentSource>(&*decoded.value);
                return value && value->discriminator == raw.get<std::string>() && value->diagnostic == decoded.diagnostic;
            }
            const auto* value = std::get_if<typed::UnknownSubAgentSource>(&*decoded.value);
            return value && raw.is_object() && raw.size() == 1 && value->discriminator == raw.begin().key() &&
                   value->diagnostic == decoded.diagnostic;
        }
        using enum detail::ConversationUnionTarget;
        switch (*target) {
            case SubAgentSourceCompact:
            case SubAgentSourceMemoryConsolidation:
            case SubAgentSourceReview: {
                const auto* value = std::get_if<typed::SubAgentSourceKind>(&*decoded.value);
                return value && raw.is_string() && value->value == raw.get<std::string>() && value->diagnostics.empty();
            }
            case SubAgentSourceOther: {
                const auto* value = std::get_if<typed::OtherSubAgentSource>(&*decoded.value);
                return value && raw.is_object() && raw.size() == 1 && raw.contains("other") &&
                       value->other == raw.at("other").get<std::string>() && value->diagnostics.empty();
            }
            case SubAgentSourceThreadSpawn: {
                const auto* value = std::get_if<typed::ThreadSpawnSubAgentSource>(&*decoded.value);
                if (value == nullptr || !raw.is_object() || !raw.contains("thread_spawn") || !raw.at("thread_spawn").is_object()) {
                    return false;
                }
                const codex::Json& nested = raw.at("thread_spawn");
                return value->threadSpawn.parentThreadId.value == nested.at("parent_thread_id").get<std::string>() &&
                       value->threadSpawn.depth == nested.at("depth").get<std::int32_t>() &&
                       nullableStringMatches(value->threadSpawn.agentPath,
                                             nested,
                                             "agent_path",
                                             [](const typed::AgentPath& path) {
                                                 return path.value;
                                             }) &&
                       nullableStringMatches(value->threadSpawn.agentNickname,
                                             nested,
                                             "agent_nickname",
                                             [](const std::string& text) {
                                                 return text;
                                             }) &&
                       nullableStringMatches(value->threadSpawn.agentRole,
                                             nested,
                                             "agent_role",
                                             [](const std::string& text) {
                                                 return text;
                                             }) &&
                       value->diagnostics.empty();
            }
            default:
                return false;
        }
    }

    bool sessionFieldsMatch(const detail::ConversationDecodeResult<typed::SessionSource>& decoded,
                            const codex::Json& raw,
                            const std::optional<detail::ConversationUnionTarget>& target) {
        if (!decoded.value || !rawMatches(decoded.value, raw)) {
            return false;
        }
        if (!target) {
            if (raw.is_string()) {
                if (!decoded.diagnostic) {
                    const auto* value = std::get_if<typed::SessionSourceKind>(&*decoded.value);
                    return value && value->value == raw.get<std::string>() && value->diagnostics.empty();
                }
                const auto* value = std::get_if<typed::UnknownSessionSourceAlternative>(&*decoded.value);
                return value && value->discriminator == raw.get<std::string>() && value->diagnostic == decoded.diagnostic;
            }
            const auto* value = std::get_if<typed::UnknownSessionSourceAlternative>(&*decoded.value);
            return value && raw.is_object() && raw.size() == 1 && value->discriminator == raw.begin().key() &&
                   value->diagnostic == decoded.diagnostic;
        }
        using enum detail::ConversationUnionTarget;
        switch (*target) {
            case SessionSourceAppServer:
            case SessionSourceCli:
            case SessionSourceExec:
            case SessionSourceUnknown:
            case SessionSourceVscode: {
                const auto* value = std::get_if<typed::SessionSourceKind>(&*decoded.value);
                return value && raw.is_string() && value->value == raw.get<std::string>() && value->diagnostics.empty();
            }
            case SessionSourceCustom: {
                const auto* value = std::get_if<typed::CustomSessionSource>(&*decoded.value);
                return value && raw.is_object() && raw.size() == 1 && raw.contains("custom") &&
                       value->custom == raw.at("custom").get<std::string>() && value->diagnostics.empty();
            }
            case SessionSourceSubAgent: {
                const auto* value = std::get_if<typed::SubAgentSessionSource>(&*decoded.value);
                if (value == nullptr || !raw.is_object() || !raw.contains("subAgent")) {
                    return false;
                }
                std::optional<typed::DecodeDiagnostic> nestedDiagnostic;
                if (const auto* unknown = std::get_if<typed::UnknownSubAgentSource>(&value->subAgent)) {
                    nestedDiagnostic = unknown->diagnostic;
                }
                detail::ConversationDecodeResult<typed::SubAgentSource> nested{value->subAgent, nestedDiagnostic};
                const bool diagnosticsMatch = decoded.diagnostic
                                                  ? value->diagnostics == std::vector<typed::DecodeDiagnostic>{*decoded.diagnostic}
                                                  : value->diagnostics.empty();
                return subAgentFieldsMatch(nested, raw.at("subAgent"), std::nullopt) && diagnosticsMatch;
            }
            default:
                return false;
        }
    }

    bool threadStatusFieldsMatch(const detail::ConversationDecodeResult<typed::ThreadStatus>& decoded,
                                 const codex::Json& raw,
                                 const std::optional<detail::ConversationUnionTarget>& target) {
        if (!decoded.value || !rawMatches(decoded.value, raw)) {
            return false;
        }
        if (!target) {
            const auto* value = std::get_if<typed::UnknownThreadStatus>(&*decoded.value);
            return value && raw.is_object() && raw.contains("type") && value->type == raw.at("type").get<std::string>() &&
                   value->diagnostic == decoded.diagnostic;
        }
        using enum detail::ConversationUnionTarget;
        switch (*target) {
            case ThreadStatusActive: {
                const auto* value = std::get_if<typed::ActiveThreadStatus>(&*decoded.value);
                if (value == nullptr || !raw.is_object() || !raw.contains("activeFlags") ||
                    value->activeFlags.size() != raw.at("activeFlags").size()) {
                    return false;
                }
                for (std::size_t index = 0; index < value->activeFlags.size(); ++index) {
                    if (value->activeFlags[index].value != raw.at("activeFlags")[index].get<std::string>()) {
                        return false;
                    }
                }
                return decoded.diagnostic ? value->diagnostics == std::vector<typed::DecodeDiagnostic>{*decoded.diagnostic}
                                          : value->diagnostics.empty();
            }
            case ThreadStatusIdle:
                return std::holds_alternative<typed::IdleThreadStatus>(*decoded.value);
            case ThreadStatusNotLoaded:
                return std::holds_alternative<typed::NotLoadedThreadStatus>(*decoded.value);
            case ThreadStatusSystemError:
                return std::holds_alternative<typed::SystemErrorThreadStatus>(*decoded.value);
            default:
                return false;
        }
    }

    struct DecodeObservation {
        std::vector<std::string> codes;
        std::optional<typed::DecodeDiagnostic> diagnostic;
        bool rawRetained = false;
        bool fieldsMatch = false;
    };

    DecodeObservation
    decodeThroughProduction(std::string_view domain, const codex::Json& raw, const std::optional<detail::ConversationUnionTarget>& target) {
        if (domain == "SessionSource") {
            auto decoded = detail::decodeSessionSource(raw);
            return {
                intrinsicCodes(decoded),
                decoded.diagnostic,
                !decoded.value || rawMatches(decoded.value, raw),
                !decoded.value || sessionFieldsMatch(decoded, raw, target),
            };
        }
        if (domain == "SubAgentSource") {
            auto decoded = detail::decodeSubAgentSource(raw);
            return {
                intrinsicCodes(decoded),
                decoded.diagnostic,
                !decoded.value || rawMatches(decoded.value, raw),
                !decoded.value || subAgentFieldsMatch(decoded, raw, target),
            };
        }
        if (domain == "ThreadStatus") {
            auto decoded = detail::decodeThreadStatus(raw);
            return {
                intrinsicCodes(decoded),
                decoded.diagnostic,
                !decoded.value || rawMatches(decoded.value, raw),
                !decoded.value || threadStatusFieldsMatch(decoded, raw, target),
            };
        }
        return {{"UnsupportedConversationDomain"}, std::nullopt, false, false};
    }

    const detail::ConversationUnionCodecDescriptor*
    descriptorFor(const detail::ProtocolSurfaceEntry& row, const std::span<const detail::ConversationUnionCodecDescriptor> descriptors) {
        const auto* target = std::get_if<detail::ConversationUnionTarget>(&row.runtimeTarget);
        if (target == nullptr) {
            return nullptr;
        }
        const auto descriptor = std::find_if(descriptors.begin(), descriptors.end(), [&](const auto& candidate) {
            return candidate.key == row.key && candidate.target == *target;
        });
        return descriptor == descriptors.end() ? nullptr : &*descriptor;
    }

    const detail::ProtocolSurfaceEntry* rowFor(const codex::Json& key) {
        if (!key.is_object() || key.value("category", "") != "tagged_union_discriminator") {
            return nullptr;
        }
        return detail::findSurface(detail::SurfaceCategory::TaggedUnionDiscriminator,
                                   key.value("domain", ""),
                                   key.value("discriminator_field", ""),
                                   key.value("name", ""));
    }
} // namespace

int main() {
    tests::support::TestResult result;
    const std::filesystem::path fixtureRoot = CODEX_A1_FIXTURE_ROOT;
    const codex::Json index = readJson(fixtureRoot / "index.json");
    const codex::Json coverage = readJson(CODEX_A1_OPERATION_PRODUCTION_COVERAGE);

    std::map<std::string, const codex::Json*> indexedById;
    for (const codex::Json& record : index.at("fixtures")) {
        indexedById.emplace(record.at("id").get<std::string>(), &record);
    }

    const std::span<const detail::ConversationUnionCodecDescriptor> descriptors = detail::conversationUnionCodecDescriptors();
    std::set<detail::ConversationUnionTarget> exactTargets;
    bool unionRegistryAgreement = true;
    bool baseFixtureAgreement = true;
    for (const codex::Json& record : coverage.at("b4_conversation_unions")) {
        const codex::Json& key = record.at("surface_key");
        const detail::ProtocolSurfaceEntry* row = rowFor(key);
        const auto* target = row == nullptr ? nullptr : std::get_if<detail::ConversationUnionTarget>(&row->runtimeTarget);
        const auto* descriptor = row == nullptr ? nullptr : descriptorFor(*row, descriptors);
        unionRegistryAgreement = unionRegistryAgreement && row && target && descriptor && row->stability == detail::Stability::Stable &&
                                 row->a1Slice == detail::A1Slice::A1_1 && row->runtimeDisposition == detail::RuntimeDisposition::Typed &&
                                 row->typedImplementation == detail::TypedImplementationStatus::Implemented &&
                                 detail::derivedTypedSchemaStatus(*row) == detail::TypedSchemaStatus::Complete &&
                                 record.at("runtime_target") == targetToken(*target) &&
                                 record.at("codec_shape") == shapeToken(descriptor->shape) &&
                                 record.at("codec_direction") == directionToken(descriptor->direction) &&
                                 descriptor->direction == detail::ConversationUnionCodecDirection::DecodeOnly;
        if (target) {
            exactTargets.emplace(*target);
        }
        const std::string baseFixtureId = record.at("base_fixture_id").get<std::string>();
        const auto base = indexedById.find(baseFixtureId);
        baseFixtureAgreement =
            baseFixtureAgreement && base != indexedById.end() && base->second->at("protocol_surface_key") == key &&
            std::find(record.at("fixture_ids").begin(), record.at("fixture_ids").end(), baseFixtureId) != record.at("fixture_ids").end();
    }
    result.expectTrue(coverage.at("b4_conversation_unions").size() == 16 && exactTargets.size() == 16,
                      "the B4 conversation corpus begins from exactly 16 distinct generated production descriptors");
    result.expectTrue(unionRegistryAgreement,
                      "all 16 descriptors agree with one exact stable complete A1.1 registry row and runtime target");
    result.expectTrue(baseFixtureAgreement, "each generated conversation descriptor names exactly its indexed positive branch fixture");

    std::map<std::string, std::size_t> roles;
    std::size_t acceptedRecords = 0;
    std::size_t malformedRecords = 0;
    std::size_t forwardCompatibleRecords = 0;
    bool exactIndexAgreement = true;
    bool exactRuntimeTargetAgreement = true;
    bool exactOutcomeAgreement = true;
    bool exactPathAgreement = true;
    bool rawRetentionAgreement = true;
    bool fieldAgreement = true;
    bool payloadRedactionAgreement = true;
    for (const codex::Json& record : coverage.at("b4_conversation_records")) {
        const std::string id = record.at("id").get<std::string>();
        const std::string role = record.at("role").get<std::string>();
        ++roles[role];
        const auto indexed = indexedById.find(id);
        exactIndexAgreement = exactIndexAgreement && indexed != indexedById.end() && indexed->second->at("file") == record.at("file") &&
                              indexed->second->at("file_sha256") == record.at("file_sha256") && indexed->second->at("role") == role;
        if (record.at("surface_key").is_null()) {
            exactIndexAgreement = exactIndexAgreement && indexed != indexedById.end() &&
                                  !indexed->second->contains("protocol_surface_key") && record.at("runtime_target").is_null();
        } else {
            exactIndexAgreement = exactIndexAgreement && indexed != indexedById.end() &&
                                  indexed->second->at("protocol_surface_key") == record.at("surface_key");
        }

        std::optional<detail::ConversationUnionTarget> target;
        if (!record.at("runtime_target").is_null()) {
            const detail::ProtocolSurfaceEntry* row = rowFor(record.at("surface_key"));
            const auto* rowTarget = row == nullptr ? nullptr : std::get_if<detail::ConversationUnionTarget>(&row->runtimeTarget);
            exactRuntimeTargetAgreement =
                exactRuntimeTargetAgreement && rowTarget && record.at("runtime_target") == targetToken(*rowTarget);
            if (rowTarget) {
                target = *rowTarget;
            }
        }

        if (indexed == indexedById.end()) {
            continue;
        }
        const codex::Json raw = readJson(fixtureRoot / record.at("file").get<std::string>());
        const DecodeObservation observation = decodeThroughProduction(record.at("domain").get<std::string>(), raw, target);
        const std::vector<std::string> expectedCodes = strings(record.at("expected_intrinsic_codes"));
        exactOutcomeAgreement = exactOutcomeAgreement && observation.codes == expectedCodes;
        exactPathAgreement = exactPathAgreement && diagnosticMatches(observation.diagnostic,
                                                                     record.at("expected_diagnostic_surface").get<std::string>(),
                                                                     record.at("expected_intrinsic_codes"),
                                                                     record.at("expected_field_path"));
        rawRetentionAgreement = rawRetentionAgreement && observation.rawRetained;
        fieldAgreement = fieldAgreement && observation.fieldsMatch;
        payloadRedactionAgreement = payloadRedactionAgreement && diagnosticOmitsPayloadValues(observation.diagnostic, raw);

        acceptedRecords += !expectedCodes.empty() && expectedCodes.front() == "Decoded";
        malformedRecords += expectedCodes == std::vector<std::string>{"MalformedKnownPayload"};
        forwardCompatibleRecords += expectedCodes == std::vector<std::string>{"Decoded", "UnknownDiscriminator"} ||
                                    expectedCodes == std::vector<std::string>{"Decoded", "UnknownEnumValue"};
        result.expectTrue(observation.codes == expectedCodes, id + " emits the exact intrinsic diagnostic-code sequence");
        result.expectTrue(diagnosticMatches(observation.diagnostic,
                                            record.at("expected_diagnostic_surface").get<std::string>(),
                                            record.at("expected_intrinsic_codes"),
                                            record.at("expected_field_path")),
                          id + " emits the exact diagnostic surface, severity, and full field path");
        result.expectTrue(observation.rawRetained, id + " retains exact raw JSON whenever decoding succeeds");
        result.expectTrue(observation.fieldsMatch, id + " selects the exact typed alternative and maps every represented field");
    }

    const std::map<std::string, std::size_t> expectedRoles = {
        {"malformed_known_conflicting_discriminators", 2},
        {"malformed_known_missing_discriminator", 8},
        {"malformed_known_missing_required", 3},
        {"malformed_known_wrong_discriminator_type", 8},
        {"malformed_known_wrong_outer_shape", 3},
        {"malformed_known_wrong_type", 7},
        {"nested_union_failure", 1},
        {"union_branch", 16},
        {"union_nullable_null", 3},
        {"union_optional_omitted", 3},
        {"unknown_discriminator", 6},
    };
    result.expectTrue(coverage.at("b4_conversation_records").size() == 60 && roles == expectedRoles && acceptedRecords == 28 &&
                          malformedRecords == 32 && forwardCompatibleRecords == 6,
                      "the exact indexed ratchet remains 60 records: 22 schema-valid, six forward-compatible, and 32 malformed-known");
    result.expectTrue(exactIndexAgreement, "all 60 production records match the indexed fixture id/path/hash/role/surface association");
    result.expectTrue(exactRuntimeTargetAgreement, "every keyed corpus record resolves through its exact generated descriptor target");
    result.expectTrue(exactOutcomeAgreement && exactPathAgreement,
                      "every record emits exactly the reviewed code sequence, severity, surface, and full field path");
    result.expectTrue(rawRetentionAgreement && fieldAgreement,
                      "every accepted union fixture retains raw JSON and maps its exact alternative fields");
    result.expectTrue(payloadRedactionAgreement, "conversation diagnostics never include string values from malformed or future payloads");

    std::map<std::string, std::size_t> activeRoles;
    bool activeIndexAgreement = true;
    bool activeOutcomeAgreement = true;
    for (const codex::Json& record : coverage.at("b4_thread_active_flag_records")) {
        const std::string id = record.at("id").get<std::string>();
        const auto indexed = indexedById.find(id);
        ++activeRoles[record.at("role").get<std::string>()];
        activeIndexAgreement = activeIndexAgreement && indexed != indexedById.end() && indexed->second->at("file") == record.at("file") &&
                               indexed->second->at("file_sha256") == record.at("file_sha256") &&
                               indexed->second->at("role") == record.at("role") &&
                               readJson(fixtureRoot / record.at("file").get<std::string>()) == record.at("value");
        const detail::ProtocolSurfaceEntry* row = rowFor(record.at("owner_surface_key"));
        const auto* target = row == nullptr ? nullptr : std::get_if<detail::ConversationUnionTarget>(&row->runtimeTarget);
        activeIndexAgreement = activeIndexAgreement && target && record.at("runtime_target") == targetToken(*target);
        if (target == nullptr) {
            continue;
        }
        const codex::Json raw = {
            {"type", "active"},
            {"activeFlags", codex::Json::array({record.at("value")})},
        };
        const DecodeObservation observation = decodeThroughProduction("ThreadStatus", raw, *target);
        const std::vector<std::string> expectedCodes = strings(record.at("expected_intrinsic_codes"));
        activeOutcomeAgreement =
            activeOutcomeAgreement && observation.codes == expectedCodes && observation.rawRetained && observation.fieldsMatch &&
            diagnosticMatches(
                observation.diagnostic, "ThreadActiveFlag", record.at("expected_intrinsic_codes"), record.at("expected_field_path")) &&
            diagnosticOmitsPayloadValues(observation.diagnostic, raw);
        result.expectTrue(observation.codes == expectedCodes, id + " preserves the exact known/future ThreadActiveFlag outcome");
    }
    result.expectTrue(coverage.at("b4_thread_active_flag_records").size() == 4 &&
                          activeRoles == std::map<std::string, std::size_t>{{"open_enum_known_value", 2}, {"unknown_enum_value", 2}},
                      "the exact ThreadActiveFlag ratchet remains two known and two future open-enum fixtures");
    result.expectTrue(activeIndexAgreement && activeOutcomeAgreement,
                      "all four indexed ThreadActiveFlag values map exactly with raw retention and full diagnostic paths");

    return result.processResult();
}
