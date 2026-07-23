/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/CodexErrorInfoCodec.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "support/TestResult.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <span>
#include <string_view>
#include <vector>

namespace {
    namespace detail = ai::openai::codex::detail;

    template <typename Target, std::size_t Size>
    void expectTargets(tests::support::TestResult& result, const std::array<std::string_view, Size>& expected, const char* description) {
        bool matches = Size == static_cast<std::size_t>(Target::Count);
        for (std::size_t index = 0; matches && index < Size; ++index) {
            const detail::ProtocolSurfaceEntry& entry = detail::entryFor(static_cast<Target>(index));
            matches = entry.key.name == expected[index] && entry.runtimeDisposition == detail::RuntimeDisposition::Typed &&
                      entry.typedImplementation == detail::TypedImplementationStatus::Implemented &&
                      entry.stability == detail::Stability::Stable;
        }
        result.expectTrue(matches, description);
    }

    bool hasExactCodes(const detail::ProtocolSurfaceValidation& validation,
                       std::initializer_list<detail::ProtocolSurfaceErrorCode> expected) {
        std::vector<detail::ProtocolSurfaceErrorCode> actualCodes;
        actualCodes.reserve(validation.errors.size());
        std::transform(validation.errors.begin(),
                       validation.errors.end(),
                       std::back_inserter(actualCodes),
                       [](const detail::ProtocolSurfaceDiagnostic& diagnostic) {
                           return diagnostic.code;
                       });
        std::vector<detail::ProtocolSurfaceErrorCode> expectedCodes(expected);
        std::sort(actualCodes.begin(), actualCodes.end());
        std::sort(expectedCodes.begin(), expectedCodes.end());
        return actualCodes == expectedCodes;
    }

    detail::SchemaCompletenessEvidence completeSchemaEvidence() {
        return {true, true, true, true, true, true, true, true, true, true, true, true, true, true};
    }

    auto findEntry(std::vector<detail::ProtocolSurfaceEntry>& entries, detail::SurfaceCategory category, std::string_view name) {
        return std::find_if(entries.begin(), entries.end(), [&](const detail::ProtocolSurfaceEntry& entry) {
            return entry.key.category == category && entry.key.name == name;
        });
    }
} // namespace

int main() {
    tests::support::TestResult result;
    const std::span<const detail::ProtocolSurfaceEntry> registry = detail::protocolSurfaceRegistry();
    const detail::ProtocolSurfaceValidation validation = detail::validateProtocolSurface(registry);

    result.expectTrue(validation && registry.size() == 387,
                      "canonical production registry is internally valid and contains all 387 pinned entries");

    std::size_t stable = 0;
    std::size_t experimentalOnly = 0;
    std::size_t normalizedFrontendEvents = 0;
    std::size_t genericFrontendExtensions = 0;
    std::size_t unknownItemMetadataSubsets = 0;
    std::size_t responseItemsNotExposed = 0;
    std::size_t stableClientAssociations = 0;
    std::size_t stableServerAssociations = 0;
    std::size_t concreteResultContracts = 0;
    std::size_t unitResultContracts = 0;
    std::size_t schemaComplete = 0;
    std::size_t schemaPartial = 0;
    std::size_t schemaNotImplemented = 0;
    std::size_t schemaNotApplicable = 0;
    std::size_t codexErrorInfoA1_0 = 0;
    std::size_t stableUnreachableInventory = 0;
    std::array<std::size_t, 7> categories{};
    std::array<std::size_t, 6> slices{};
    for (const detail::ProtocolSurfaceEntry& entry : registry) {
        entry.stability == detail::Stability::Stable ? ++stable : ++experimentalOnly;
        ++categories[static_cast<std::size_t>(entry.key.category)];
        if (entry.stability == detail::Stability::Stable) {
            normalizedFrontendEvents += entry.frontendProtocol == detail::FrontendExposure::ExistingEventSubset;
            genericFrontendExtensions += entry.frontendProtocol == detail::FrontendExposure::GenericExtension;
            unknownItemMetadataSubsets += entry.frontendProtocol == detail::FrontendExposure::ExistingUnknownItemSubset;
            responseItemsNotExposed += entry.key.category == detail::SurfaceCategory::ItemDiscriminator &&
                                       entry.key.domain == "ResponseItem" && entry.frontendProtocol == detail::FrontendExposure::NotExposed;
            stableClientAssociations += entry.key.category == detail::SurfaceCategory::ClientRequest &&
                                        entry.operationContract.evidenceKind == detail::AssociationEvidenceKind::VendoredRust;
            stableServerAssociations += entry.key.category == detail::SurfaceCategory::ServerRequest &&
                                        entry.operationContract.evidenceKind == detail::AssociationEvidenceKind::VendoredSchemaPair;
            concreteResultContracts += (entry.key.category == detail::SurfaceCategory::ClientRequest ||
                                        entry.key.category == detail::SurfaceCategory::ServerRequest) &&
                                       entry.operationContract.resultKind == detail::ResultContractKind::Concrete;
            unitResultContracts += (entry.key.category == detail::SurfaceCategory::ClientRequest ||
                                    entry.key.category == detail::SurfaceCategory::ServerRequest) &&
                                   entry.operationContract.resultKind == detail::ResultContractKind::Unit;
        }
        switch (entry.typedSchemaStatus) {
            case detail::TypedSchemaStatus::Complete:
                ++schemaComplete;
                break;
            case detail::TypedSchemaStatus::Partial:
                ++schemaPartial;
                break;
            case detail::TypedSchemaStatus::NotImplemented:
                ++schemaNotImplemented;
                break;
            case detail::TypedSchemaStatus::NotApplicable:
                ++schemaNotApplicable;
                break;
        }
        switch (entry.a1Slice) {
            case detail::A1Slice::A1_0:
                ++slices[0];
                break;
            case detail::A1Slice::A1_1:
                ++slices[1];
                break;
            case detail::A1Slice::A1_2:
                ++slices[2];
                break;
            case detail::A1Slice::A1_3:
                ++slices[3];
                break;
            case detail::A1Slice::A1_4:
                ++slices[4];
                break;
            case detail::A1Slice::InventoryOnly:
                ++slices[5];
                break;
            case detail::A1Slice::Unassigned:
                break;
        }
        codexErrorInfoA1_0 += entry.key.domain == "CodexErrorInfo" && entry.a1Slice == detail::A1Slice::A1_0;
        stableUnreachableInventory += entry.stability == detail::Stability::Stable && entry.typedModule == "StableUnreachableInventory" &&
                                      entry.a1Slice == detail::A1Slice::InventoryOnly;
    }
    result.expectTrue(stable == 351 && experimentalOnly == 36, "registry keeps stable and experimental-only membership distinct");
    result.expectTrue(categories[static_cast<std::size_t>(detail::SurfaceCategory::ClientNotification)] == 1 &&
                          categories[static_cast<std::size_t>(detail::SurfaceCategory::ClientRequest)] == 122 &&
                          categories[static_cast<std::size_t>(detail::SurfaceCategory::DeltaProgressDiscriminator)] == 0 &&
                          categories[static_cast<std::size_t>(detail::SurfaceCategory::ItemDiscriminator)] == 34 &&
                          categories[static_cast<std::size_t>(detail::SurfaceCategory::ServerNotification)] == 68 &&
                          categories[static_cast<std::size_t>(detail::SurfaceCategory::ServerRequest)] == 11 &&
                          categories[static_cast<std::size_t>(detail::SurfaceCategory::TaggedUnionDiscriminator)] == 151,
                      "registry category counts match the pinned schema census");
    result.expectTrue(normalizedFrontendEvents == 22 && genericFrontendExtensions == 54 && unknownItemMetadataSubsets == 10 &&
                          responseItemsNotExposed == 16,
                      "frontend event dispositions distinguish normalized subsets, raw extensions, metadata-only unknown items, and "
                      "undispatched response items");
    result.expectTrue(stableClientAssociations == 87 && stableServerAssociations == 10,
                      "canonical registry carries all 87 Rust-derived client contracts and all 10 schema-paired server contracts");
    result.expectTrue(concreteResultContracts == 76 && unitResultContracts == 21,
                      "result contracts preserve 76 concrete and 21 explicit Unit identities without empty-string sentinels");
    result.expectTrue(schemaComplete == 16 && schemaPartial == 34 && schemaNotImplemented == 289 && schemaNotApplicable == 48,
                      "A1.0 completes exactly 16 CodexErrorInfo identities while the existing 34 identities remain honestly partial");
    result.expectTrue(slices == std::array<std::size_t, 6>{19, 151, 45, 68, 56, 48} && codexErrorInfoA1_0 == 16 &&
                          stableUnreachableInventory == 12,
                      "registry preserves the frozen A1 slice assignment, CodexErrorInfo exception, and 12 stable unreachable rows");
    result.expectTrue(std::all_of(registry.begin(),
                                  registry.end(),
                                  [](const detail::ProtocolSurfaceEntry& entry) {
                                      return !entry.typedModule.empty() && entry.a1Slice != detail::A1Slice::Unassigned &&
                                             entry.typedSchemaStatus == detail::derivedTypedSchemaStatus(entry);
                                  }),
                      "every identity has one fixed module/slice assignment and a mechanically derived schema status");

    expectTargets<detail::ClientRequestTarget>(
        result,
        std::array<std::string_view, 7>{
            "initialize", "thread/start", "thread/resume", "thread/list", "thread/read", "turn/start", "turn/interrupt"},
        "every existing typed outgoing request target resolves to its exact registered wire method");
    expectTargets<detail::ClientNotificationTarget>(result,
                                                    std::array<std::string_view, 1>{"initialized"},
                                                    "the typed outgoing notification target resolves to its exact registered wire method");
    expectTargets<detail::ServerNotificationTarget>(
        result,
        std::array<std::string_view, 14>{"error",
                                         "thread/started",
                                         "thread/status/changed",
                                         "turn/started",
                                         "turn/completed",
                                         "item/started",
                                         "item/completed",
                                         "item/agentMessage/delta",
                                         "item/reasoning/textDelta",
                                         "item/reasoning/summaryTextDelta",
                                         "item/commandExecution/outputDelta",
                                         "item/fileChange/patchUpdated",
                                         "thread/tokenUsage/updated",
                                         "model/rerouted"},
        "every existing typed notification dispatch target resolves to its exact registered wire method");
    expectTargets<detail::ServerRequestTarget>(
        result,
        std::array<std::string_view, 4>{"item/commandExecution/requestApproval",
                                        "item/fileChange/requestApproval",
                                        "item/tool/requestUserInput",
                                        "account/chatgptAuthTokens/refresh"},
        "every existing typed server-request dispatch target resolves to its exact registered wire method");
    expectTargets<detail::ItemDiscriminatorTarget>(
        result,
        std::array<std::string_view, 8>{
            "agentMessage", "userMessage", "reasoning", "commandExecution", "fileChange", "mcpToolCall", "dynamicToolCall", "webSearch"},
        "every existing typed item dispatch target resolves to its exact registered discriminator");
    expectTargets<detail::CodexErrorInfoTarget>(
        result,
        std::array<std::string_view, 16>{"activeTurnNotSteerable",
                                         "badRequest",
                                         "contextWindowExceeded",
                                         "cyberPolicy",
                                         "httpConnectionFailed",
                                         "internalServerError",
                                         "other",
                                         "responseStreamConnectionFailed",
                                         "responseStreamDisconnected",
                                         "responseTooManyFailedAttempts",
                                         "sandboxError",
                                         "serverOverloaded",
                                         "sessionBudgetExceeded",
                                         "threadRollbackFailed",
                                         "unauthorized",
                                         "usageLimitExceeded"},
        "every CodexErrorInfo decoder target resolves to its exact canonical tagged-union identity");

    const std::span<const detail::CodexErrorInfoCodecDescriptor> codecDescriptors =
        detail::codexErrorInfoCodecDescriptors();
    result.expectTrue(codecDescriptors.size() == 16,
                      "the production CodexErrorInfo decoder exposes exactly 16 private descriptors");

    std::vector<detail::CodexErrorInfoCodecDescriptor> duplicateCodecDescriptors(codecDescriptors.begin(), codecDescriptors.end());
    duplicateCodecDescriptors.push_back(codecDescriptors.front());
    result.expectTrue(
        hasExactCodes(detail::validateProtocolSurface(registry, duplicateCodecDescriptors),
                      {detail::ProtocolSurfaceErrorCode::DuplicateCodecDescriptor}),
        "duplicate private decoder descriptor fails with exactly DuplicateCodecDescriptor");

    std::vector<detail::CodexErrorInfoCodecDescriptor> missingCodecDescriptor(codecDescriptors.begin(), codecDescriptors.end());
    missingCodecDescriptor.erase(missingCodecDescriptor.begin());
    result.expectTrue(
        hasExactCodes(detail::validateProtocolSurface(registry, missingCodecDescriptor),
                      {detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                       detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor}),
        "a complete typed row without a production decoder fails with exact row-without-decoder diagnostics");

    std::vector<detail::CodexErrorInfoCodecDescriptor> staleCodecDescriptor(codecDescriptors.begin(), codecDescriptors.end());
    staleCodecDescriptor.front().key.name = "staleCodexErrorInfo";
    result.expectTrue(
        hasExactCodes(detail::validateProtocolSurface(registry, staleCodecDescriptor),
                      {detail::ProtocolSurfaceErrorCode::CodecDescriptorWithoutRegistryRow,
                       detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                       detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor}),
        "a production decoder descriptor without an exact registry row fails with the exact stale-descriptor diagnostics");

    std::vector<detail::ProtocolSurfaceEntry> targetWithoutImplementedRow(registry.begin(), registry.end());
    const auto unimplementedTarget =
        findEntry(targetWithoutImplementedRow, detail::SurfaceCategory::TaggedUnionDiscriminator, "activeTurnNotSteerable");
    unimplementedTarget->runtimeDisposition = detail::RuntimeDisposition::Deferred;
    unimplementedTarget->typedImplementation = detail::TypedImplementationStatus::NotImplemented;
    unimplementedTarget->typedSchemaStatus = detail::TypedSchemaStatus::NotImplemented;
    unimplementedTarget->schemaCompleteness = {};
    result.expectTrue(
        hasExactCodes(detail::validateProtocolSurface(targetWithoutImplementedRow),
                      {detail::ProtocolSurfaceErrorCode::RuntimeTargetWithoutTypedImplementation,
                       detail::ProtocolSurfaceErrorCode::CodecDescriptorWithoutTypedRegistryRow}),
        "a nested-union target without an implemented row fails with exact target/disposition diagnostics");

    std::vector<detail::ProtocolSurfaceEntry> completeRowWithoutTarget(registry.begin(), registry.end());
    const auto targetlessComplete =
        findEntry(completeRowWithoutTarget, detail::SurfaceCategory::TaggedUnionDiscriminator, "activeTurnNotSteerable");
    targetlessComplete->runtimeTarget = std::monostate{};
    result.expectTrue(
        hasExactCodes(detail::validateProtocolSurface(completeRowWithoutTarget),
                      {detail::ProtocolSurfaceErrorCode::TypedWithoutRuntimeTarget,
                       detail::ProtocolSurfaceErrorCode::MissingRuntimeTargetRegistration,
                       detail::ProtocolSurfaceErrorCode::CodecDescriptorTargetMismatch,
                       detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                       detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor}),
        "a complete nested-union row without target/decoder fails with the exact bidirectional diagnostic multiset");

    const detail::ProtocolSurfaceEntry* deferred =
        detail::findSurface(detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "thread/archive");
    result.expectTrue(deferred && deferred->runtimeDisposition == detail::RuntimeDisposition::Deferred &&
                          deferred->typedImplementation == detail::TypedImplementationStatus::NotImplemented &&
                          std::holds_alternative<std::monostate>(deferred->runtimeTarget),
                      "registered-but-unimplemented requests remain explicitly deferred rather than typed");
    result.expectTrue(detail::findSurface(detail::SurfaceCategory::ServerNotification, "ServerNotification", "method", "future/unknown") ==
                          nullptr,
                      "future methods absent from the pinned upstream surface remain distinguishable from registered entries");

    const detail::ProtocolSurfaceEntry* unknownThreadItem =
        detail::findSurface(detail::SurfaceCategory::ItemDiscriminator, "ThreadItem", "type", "plan");
    result.expectTrue(unknownThreadItem && unknownThreadItem->frontendProtocol == detail::FrontendExposure::ExistingUnknownItemSubset &&
                          unknownThreadItem->frontendSecurity == detail::FrontendSecurityDecision::ExistingUnknownItemMetadataContract,
                      "untyped ThreadItem entries claim only the existing metadata-only frontend subset");

    const detail::ProtocolSurfaceEntry* responseItem =
        detail::findSurface(detail::SurfaceCategory::ItemDiscriminator, "ResponseItem", "type", "message");
    result.expectTrue(responseItem && responseItem->frontendProtocol == detail::FrontendExposure::NotExposed &&
                          responseItem->frontendSecurity == detail::FrontendSecurityDecision::Unresolved,
                      "undispatched ResponseItem entries do not claim an existing frontend payload path");

    std::vector<detail::ProtocolSurfaceEntry> wrongUnknownItemPair(registry.begin(), registry.end());
    const auto wrongUnknownItem =
        std::find_if(wrongUnknownItemPair.begin(), wrongUnknownItemPair.end(), [](const detail::ProtocolSurfaceEntry& entry) {
            return entry.frontendProtocol == detail::FrontendExposure::ExistingUnknownItemSubset;
        });
    if (wrongUnknownItem == wrongUnknownItemPair.end()) {
        result.expectTrue(false, "registry contains an unknown-item metadata subset for negative validation");
    } else {
        wrongUnknownItem->frontendSecurity = detail::FrontendSecurityDecision::ExistingRedactedExtensionContract;
        result.expectTrue(
            hasExactCodes(detail::validateProtocolSurface(wrongUnknownItemPair),
                          {detail::ProtocolSurfaceErrorCode::FrontendSecurityMismatch}),
            "registry validation reports only FrontendSecurityMismatch for an unknown-item subset mislabeled as a raw extension contract");
    }

    std::vector<detail::ProtocolSurfaceEntry> missingAssociation(registry.begin(), registry.end());
    const auto missingContract = findEntry(missingAssociation, detail::SurfaceCategory::ClientRequest, "thread/archive");
    missingContract->operationContract = {};
    result.expectTrue(
        hasExactCodes(detail::validateProtocolSurface(missingAssociation), {detail::ProtocolSurfaceErrorCode::MissingAssociation}),
        "registry validation reports only MissingAssociation when one stable request loses its authoritative contract");

    std::vector<detail::ProtocolSurfaceEntry> duplicateAssociation(registry.begin(), registry.end());
    const auto firstAssociation =
        findEntry(duplicateAssociation, detail::SurfaceCategory::ClientRequest, "thread/archive");
    const auto secondAssociation =
        findEntry(duplicateAssociation, detail::SurfaceCategory::ClientRequest, "thread/compact/start");
    secondAssociation->operationContract.evidenceKind = firstAssociation->operationContract.evidenceKind;
    secondAssociation->operationContract.evidenceKey = firstAssociation->operationContract.evidenceKey;
    result.expectTrue(
        hasExactCodes(detail::validateProtocolSurface(duplicateAssociation),
                      {detail::ProtocolSurfaceErrorCode::DuplicateAssociation}),
        "registry validation reports only DuplicateAssociation when two exact rows reuse authoritative evidence");

    std::vector<detail::ProtocolSurfaceEntry> conflictingEvidence(registry.begin(), registry.end());
    const auto conflictingContract = findEntry(conflictingEvidence, detail::SurfaceCategory::ClientRequest, "thread/archive");
    conflictingContract->operationContract.evidenceKind = detail::AssociationEvidenceKind::VendoredSchemaPair;
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(conflictingEvidence),
                                    {detail::ProtocolSurfaceErrorCode::ConflictingAssociationEvidence}),
                      "registry validation reports only ConflictingAssociationEvidence for a client contract attributed to schema pairing");

    std::vector<detail::ProtocolSurfaceEntry> unitMismatch(registry.begin(), registry.end());
    const auto nonUnit = findEntry(unitMismatch, detail::SurfaceCategory::ClientRequest, "thread/archive");
    nonUnit->operationContract.resultKind = detail::ResultContractKind::Unit;
    nonUnit->operationContract.resultTypeIdentity = "ThreadArchiveResponse";
    result.expectTrue(
        hasExactCodes(detail::validateProtocolSurface(unitMismatch), {detail::ProtocolSurfaceErrorCode::UnitWithNonUnitResultType}),
        "registry validation reports only UnitWithNonUnitResultType for an inconsistent explicit unit contract");

    std::vector<detail::ProtocolSurfaceEntry> concreteWithoutType(registry.begin(), registry.end());
    const auto emptyConcrete = findEntry(concreteWithoutType, detail::SurfaceCategory::ClientRequest, "thread/archive");
    emptyConcrete->operationContract.resultKind = detail::ResultContractKind::Concrete;
    emptyConcrete->operationContract.resultTypeIdentity = {};
    result.expectTrue(
        hasExactCodes(detail::validateProtocolSurface(concreteWithoutType), {detail::ProtocolSurfaceErrorCode::ConcreteWithoutResultType}),
        "registry validation reports only ConcreteWithoutResultType for an empty concrete result identity");

    std::vector<detail::ProtocolSurfaceEntry> contractOnNonRequest(registry.begin(), registry.end());
    const auto nonRequest = findEntry(contractOnNonRequest, detail::SurfaceCategory::ServerNotification, "thread/closed");
    nonRequest->operationContract =
        findEntry(contractOnNonRequest, detail::SurfaceCategory::ClientRequest, "thread/archive")->operationContract;
    result.expectTrue(
        hasExactCodes(detail::validateProtocolSurface(contractOnNonRequest), {detail::ProtocolSurfaceErrorCode::ContractOnNonRequest}),
        "registry validation reports only ContractOnNonRequest when a notification acquires request contract data");

    std::vector<detail::ProtocolSurfaceEntry> experimentalAssociation(registry.begin(), registry.end());
    const auto experimental = findEntry(experimentalAssociation, detail::SurfaceCategory::ClientRequest, "collaborationMode/list");
    experimental->operationContract =
        findEntry(experimentalAssociation, detail::SurfaceCategory::ClientRequest, "thread/archive")->operationContract;
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(experimentalAssociation),
                                    {detail::ProtocolSurfaceErrorCode::ExperimentalAssociationCountedAsStable}),
                      "registry validation reports only ExperimentalAssociationCountedAsStable for inventory-only evidence");

    std::vector<detail::ProtocolSurfaceEntry> missingModule(registry.begin(), registry.end());
    findEntry(missingModule, detail::SurfaceCategory::ClientRequest, "thread/archive")->typedModule = {};
    result.expectTrue(
        hasExactCodes(detail::validateProtocolSurface(missingModule), {detail::ProtocolSurfaceErrorCode::MissingTypedModuleAssignment}),
        "registry validation reports only MissingTypedModuleAssignment for one unassigned typed module");

    std::vector<detail::ProtocolSurfaceEntry> missingSlice(registry.begin(), registry.end());
    findEntry(missingSlice, detail::SurfaceCategory::ClientRequest, "thread/archive")->a1Slice = detail::A1Slice::Unassigned;
    result.expectTrue(
        hasExactCodes(detail::validateProtocolSurface(missingSlice), {detail::ProtocolSurfaceErrorCode::MissingSliceAssignment}),
        "registry validation reports only MissingSliceAssignment for one unassigned A1 identity");

    const auto expectCompletenessMutation = [&](auto mutate, detail::ProtocolSurfaceErrorCode expected, const char* description) {
        std::vector<detail::ProtocolSurfaceEntry> entries(registry.begin(), registry.end());
        const auto candidate = findEntry(entries, detail::SurfaceCategory::ClientRequest, "initialize");
        candidate->schemaCompleteness = completeSchemaEvidence();
        candidate->typedSchemaStatus = detail::TypedSchemaStatus::Complete;
        mutate(candidate->schemaCompleteness);
        result.expectTrue(hasExactCodes(detail::validateProtocolSurface(entries), {expected}), description);
    };
    expectCompletenessMutation(
        [](detail::SchemaCompletenessEvidence& evidence) {
            evidence.authoritativeRootAssociation = false;
        },
        detail::ProtocolSurfaceErrorCode::ClaimedCompleteWithoutAuthoritativeAssociation,
        "complete status requires an authoritative schema/root association");
    expectCompletenessMutation(
        [](detail::SchemaCompletenessEvidence& evidence) {
            evidence.positiveFixtureCoverage = false;
        },
        detail::ProtocolSurfaceErrorCode::ClaimedCompleteWithoutPositiveFixtureCoverage,
        "complete status requires positive schema-derived fixture coverage");
    expectCompletenessMutation(
        [](detail::SchemaCompletenessEvidence& evidence) {
            evidence.requiredFieldsExercised = false;
        },
        detail::ProtocolSurfaceErrorCode::RequiredFieldNotExercised,
        "complete status requires every required field to be exercised");
    expectCompletenessMutation(
        [](detail::SchemaCompletenessEvidence& evidence) {
            evidence.schemaPropertiesExercised = false;
        },
        detail::ProtocolSurfaceErrorCode::SchemaPropertyNotExercised,
        "complete status requires every represented schema property to be exercised");
    expectCompletenessMutation(
        [](detail::SchemaCompletenessEvidence& evidence) {
            evidence.optionalPresentExercised = false;
        },
        detail::ProtocolSurfaceErrorCode::OptionalPresentCaseMissing,
        "complete status requires optional-present coverage");
    expectCompletenessMutation(
        [](detail::SchemaCompletenessEvidence& evidence) {
            evidence.optionalOmittedExercised = false;
        },
        detail::ProtocolSurfaceErrorCode::OptionalOmittedCaseMissing,
        "complete status requires optional-omitted coverage");
    expectCompletenessMutation(
        [](detail::SchemaCompletenessEvidence& evidence) {
            evidence.nullableSemanticsExercised = false;
        },
        detail::ProtocolSurfaceErrorCode::NullableSemanticsMissing,
        "complete status requires distinct nullable/null/value/omitted coverage");
    expectCompletenessMutation(
        [](detail::SchemaCompletenessEvidence& evidence) {
            evidence.reachableUnionAlternativesExercised = false;
        },
        detail::ProtocolSurfaceErrorCode::ReachableUnionAlternativeMissing,
        "complete status requires every reachable known union alternative");
    expectCompletenessMutation(
        [](detail::SchemaCompletenessEvidence& evidence) {
            evidence.directionAssertionsExercised = false;
        },
        detail::ProtocolSurfaceErrorCode::DirectionAssertionMissing,
        "complete status requires direction-specific codec assertions");
    expectCompletenessMutation(
        [](detail::SchemaCompletenessEvidence& evidence) {
            evidence.fixtureCurrent = false;
        },
        detail::ProtocolSurfaceErrorCode::StaleFixture,
        "complete status rejects stale fixture evidence");
    expectCompletenessMutation(
        [](detail::SchemaCompletenessEvidence& evidence) {
            evidence.runtimeDecoderMatchesRegistry = false;
        },
        detail::ProtocolSurfaceErrorCode::CompletenessRuntimeTargetMismatch,
        "complete status requires the production target/decoder identity to match the registry");
    expectCompletenessMutation(
        [](detail::SchemaCompletenessEvidence& evidence) {
            evidence.opaqueFieldsDeclared = false;
        },
        detail::ProtocolSurfaceErrorCode::UnrecordedOpaqueField,
        "complete status rejects an unrecorded protocol-opaque field");
    expectCompletenessMutation(
        [](detail::SchemaCompletenessEvidence& evidence) {
            evidence.independentlySchemaValidated = false;
        },
        detail::ProtocolSurfaceErrorCode::ClaimedCompleteWithoutIndependentValidation,
        "complete status requires independent schema validation");
    expectCompletenessMutation(
        [](detail::SchemaCompletenessEvidence& evidence) {
            evidence.noKnownSchemaFieldsDropped = false;
        },
        detail::ProtocolSurfaceErrorCode::KnownSchemaFieldDropped,
        "complete status rejects silently dropped known schema fields");

    std::vector<detail::ProtocolSurfaceEntry> understatedCompleteness(registry.begin(), registry.end());
    const auto understated = findEntry(understatedCompleteness, detail::SurfaceCategory::ClientRequest, "initialize");
    understated->schemaCompleteness = completeSchemaEvidence();
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(understatedCompleteness),
                                    {detail::ProtocolSurfaceErrorCode::TypedSchemaStatusMismatch}),
                      "a registry enum cannot keep a mechanically complete evidence row classified as Partial");

    std::vector<detail::ProtocolSurfaceEntry> overstatedNotImplemented(registry.begin(), registry.end());
    findEntry(overstatedNotImplemented, detail::SurfaceCategory::ClientRequest, "thread/archive")->typedSchemaStatus =
        detail::TypedSchemaStatus::Partial;
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(overstatedNotImplemented),
                                    {detail::ProtocolSurfaceErrorCode::TypedSchemaStatusMismatch}),
                      "a registry enum cannot promote a mechanically NotImplemented row to Partial");

    std::vector<detail::ProtocolSurfaceEntry> overstatedNotApplicable(registry.begin(), registry.end());
    findEntry(overstatedNotApplicable, detail::SurfaceCategory::ClientRequest, "collaborationMode/list")->typedSchemaStatus =
        detail::TypedSchemaStatus::Partial;
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(overstatedNotApplicable),
                                    {detail::ProtocolSurfaceErrorCode::TypedSchemaStatusMismatch}),
                      "a registry enum cannot promote a mechanically NotApplicable inventory row to Partial");

    return result.processResult();
}
