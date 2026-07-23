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
#include <string>
#include <string_view>
#include <utility>
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

    auto findExactEntry(std::vector<detail::ProtocolSurfaceEntry>& entries, const detail::ProtocolSurfaceKey& key) {
        return std::find_if(entries.begin(), entries.end(), [&](const detail::ProtocolSurfaceEntry& entry) {
            return entry.key == key;
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
    result.expectTrue(schemaComplete == 92 && schemaPartial == 26 && schemaNotImplemented == 221 && schemaNotApplicable == 48,
                      "the staged B3 registry advances exactly 50 item-batch identities without promoting later batches");
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
        std::array<std::string_view, 18>{"agentMessage",
                                         "collabAgentToolCall",
                                         "commandExecution",
                                         "contextCompaction",
                                         "dynamicToolCall",
                                         "enteredReviewMode",
                                         "exitedReviewMode",
                                         "fileChange",
                                         "hookPrompt",
                                         "imageGeneration",
                                         "imageView",
                                         "mcpToolCall",
                                         "plan",
                                         "reasoning",
                                         "sleep",
                                         "subAgentActivity",
                                         "userMessage",
                                         "webSearch"},
        "every ThreadItem dispatch target resolves to its exact registered discriminator");
    expectTargets<detail::ResponseItemTarget>(
        result,
        std::array<std::string_view, 16>{"agent_message",
                                         "compaction",
                                         "compaction_trigger",
                                         "context_compaction",
                                         "custom_tool_call",
                                         "custom_tool_call_output",
                                         "function_call",
                                         "function_call_output",
                                         "image_generation_call",
                                         "local_shell_call",
                                         "message",
                                         "other",
                                         "reasoning",
                                         "tool_search_call",
                                         "tool_search_output",
                                         "web_search_call"},
        "every ResponseItem dispatch target resolves to its distinct exact registered discriminator");
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
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(registry, staleCodecDescriptor),
                                    {detail::ProtocolSurfaceErrorCode::CodecDescriptorCanonicalKeyMismatch,
                                     detail::ProtocolSurfaceErrorCode::CodecDescriptorWithoutRegistryRow,
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

    const std::span<const detail::ConversationUnionCodecDescriptor> conversationDescriptors = detail::conversationUnionCodecDescriptors();
    constexpr std::array<detail::ProtocolSurfaceKey, 42> expectedConversationTargets{{
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "AgentMessageInputContent", "type", "encrypted_content"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "AgentMessageInputContent", "type", "input_text"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "AskForApproval", "$variant", "granular"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "AskForApproval", "$variant", "never"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "AskForApproval", "$variant", "on-request"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "AskForApproval", "$variant", "untrusted"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "CommandAction", "type", "listFiles"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "CommandAction", "type", "read"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "CommandAction", "type", "search"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "CommandAction", "type", "unknown"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "ContentItem", "type", "input_image"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "ContentItem", "type", "input_text"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "ContentItem", "type", "output_text"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "DynamicToolCallOutputContentItem", "type", "inputImage"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "DynamicToolCallOutputContentItem", "type", "inputText"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "FunctionCallOutputContentItem", "type", "encrypted_content"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "FunctionCallOutputContentItem", "type", "input_image"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "FunctionCallOutputContentItem", "type", "input_text"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "LocalShellAction", "type", "exec"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "PatchChangeKind", "type", "add"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "PatchChangeKind", "type", "delete"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "PatchChangeKind", "type", "update"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "ReasoningItemContent", "type", "reasoning_text"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "ReasoningItemContent", "type", "text"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "ReasoningItemReasoningSummary", "type", "summary_text"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "ResponsesApiWebSearchAction", "type", "find_in_page"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "ResponsesApiWebSearchAction", "type", "open_page"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "ResponsesApiWebSearchAction", "type", "other"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "ResponsesApiWebSearchAction", "type", "search"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "SandboxPolicy", "type", "dangerFullAccess"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "SandboxPolicy", "type", "externalSandbox"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "SandboxPolicy", "type", "readOnly"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "SandboxPolicy", "type", "workspaceWrite"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "UserInput", "type", "image"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "UserInput", "type", "localImage"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "UserInput", "type", "mention"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "UserInput", "type", "skill"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "UserInput", "type", "text"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "WebSearchAction", "type", "findInPage"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "WebSearchAction", "type", "openPage"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "WebSearchAction", "type", "other"},
        {detail::SurfaceCategory::TaggedUnionDiscriminator, "WebSearchAction", "type", "search"},
    }};
    bool exactConversationTargets = conversationDescriptors.size() == expectedConversationTargets.size() &&
                                    conversationDescriptors.size() == static_cast<std::size_t>(detail::ConversationUnionTarget::Count);
    for (std::size_t index = 0; exactConversationTargets && index < conversationDescriptors.size(); ++index) {
        const auto target = static_cast<detail::ConversationUnionTarget>(index);
        exactConversationTargets = conversationDescriptors[index].key == expectedConversationTargets[index] &&
                                   conversationDescriptors[index].target == target &&
                                   detail::entryFor(target).key == expectedConversationTargets[index];
    }
    result.expectTrue(exactConversationTargets,
                      "the generated dependency-closed B2+B3 descriptor artifact maps exactly 42 canonical keys to 42 stable targets");

    const std::vector<detail::ProtocolSurfaceEntry> conversationRegistry(registry.begin(), registry.end());
    result.expectTrue(static_cast<bool>(detail::validateProtocolSurface(registry)),
                      "the canonical registry validates all 42 B2+B3 nested rows against generated production descriptors");
    result.expectTrue(static_cast<bool>(detail::validateProtocolSurface(
                          conversationRegistry, detail::codexErrorInfoCodecDescriptors(), conversationDescriptors)),
                      "the exact 42-target descriptor set validates bidirectionally against the canonical registry");

    std::vector<detail::ConversationUnionCodecDescriptor> duplicateConversationDescriptor(conversationDescriptors.begin(),
                                                                                          conversationDescriptors.end());
    duplicateConversationDescriptor.push_back(conversationDescriptors.front());
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(
                                        conversationRegistry, detail::codexErrorInfoCodecDescriptors(), duplicateConversationDescriptor),
                                    {detail::ProtocolSurfaceErrorCode::DuplicateCodecDescriptor}),
                      "a duplicate conversation descriptor fails with exactly DuplicateCodecDescriptor");

    std::vector<detail::ConversationUnionCodecDescriptor> missingConversationDescriptor(conversationDescriptors.begin(),
                                                                                        conversationDescriptors.end());
    missingConversationDescriptor.erase(missingConversationDescriptor.begin());
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(
                                        conversationRegistry, detail::codexErrorInfoCodecDescriptors(), missingConversationDescriptor),
                                    {detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                                     detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor}),
                      "a typed complete conversation row without its descriptor fails with the exact missing-descriptor codes");

    std::vector<detail::ConversationUnionCodecDescriptor> staleConversationDescriptor(conversationDescriptors.begin(),
                                                                                      conversationDescriptors.end());
    staleConversationDescriptor.front().key.name = "staleConversationUnion";
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(
                                        conversationRegistry, detail::codexErrorInfoCodecDescriptors(), staleConversationDescriptor),
                                    {detail::ProtocolSurfaceErrorCode::CodecDescriptorWithoutRegistryRow,
                                     detail::ProtocolSurfaceErrorCode::CodecDescriptorCanonicalKeyMismatch,
                                     detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                                     detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor}),
                      "a stale conversation descriptor fails with the exact bidirectional key-association codes");

    std::vector<detail::ConversationUnionCodecDescriptor> mismatchedConversationTargets(conversationDescriptors.begin(),
                                                                                        conversationDescriptors.end());
    std::swap(mismatchedConversationTargets[0].target, mismatchedConversationTargets[1].target);
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(
                                        conversationRegistry, detail::codexErrorInfoCodecDescriptors(), mismatchedConversationTargets),
                                    {detail::ProtocolSurfaceErrorCode::CodecDescriptorTargetMismatch,
                                     detail::ProtocolSurfaceErrorCode::CodecDescriptorTargetMismatch,
                                     detail::ProtocolSurfaceErrorCode::CodecDescriptorCanonicalKeyMismatch,
                                     detail::ProtocolSurfaceErrorCode::CodecDescriptorCanonicalKeyMismatch,
                                     detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                                     detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                                     detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor,
                                     detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor}),
                      "descriptor target swaps fail with the exact target/key mismatch multiset");

    std::vector<detail::ConversationUnionCodecDescriptor> invalidConversationShape(conversationDescriptors.begin(),
                                                                                   conversationDescriptors.end());
    invalidConversationShape.front().shape = detail::ConversationUnionCodecShape::ScalarString;
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(
                                        conversationRegistry, detail::codexErrorInfoCodecDescriptors(), invalidConversationShape),
                                    {detail::ProtocolSurfaceErrorCode::InvalidCodecDescriptorShape}),
                      "a field-compatible but wrong descriptor shape fails with exactly InvalidCodecDescriptorShape");

    std::vector<detail::ConversationUnionCodecDescriptor> invalidConversationDirection(conversationDescriptors.begin(),
                                                                                       conversationDescriptors.end());
    invalidConversationDirection[2].direction = detail::ConversationUnionCodecDirection::DecodeOnly;
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(
                                        conversationRegistry, detail::codexErrorInfoCodecDescriptors(), invalidConversationDirection),
                                    {detail::ProtocolSurfaceErrorCode::InvalidCodecDescriptorDirection}),
                      "a valid but wrong codec direction fails with exactly InvalidCodecDescriptorDirection");

    std::vector<detail::ProtocolSurfaceEntry> retainedConversationTarget = conversationRegistry;
    const auto retainedConversationRow = findExactEntry(retainedConversationTarget, conversationDescriptors.front().key);
    retainedConversationRow->runtimeDisposition = detail::RuntimeDisposition::OpaquePreserved;
    retainedConversationRow->typedImplementation = detail::TypedImplementationStatus::NotImplemented;
    retainedConversationRow->typedSchemaStatus = detail::TypedSchemaStatus::NotImplemented;
    retainedConversationRow->schemaCompleteness = {};
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(
                                        retainedConversationTarget, detail::codexErrorInfoCodecDescriptors(), conversationDescriptors),
                                    {detail::ProtocolSurfaceErrorCode::RuntimeTargetWithoutTypedImplementation,
                                     detail::ProtocolSurfaceErrorCode::CodecDescriptorWithoutTypedRegistryRow}),
                      "a retained conversation target on a demoted row fails with the exact target/descriptor codes");

    std::vector<detail::ProtocolSurfaceEntry> missingConversationTarget = conversationRegistry;
    const auto missingConversationTargetRow = findExactEntry(missingConversationTarget, conversationDescriptors.front().key);
    missingConversationTargetRow->runtimeTarget = std::monostate{};
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(
                                        missingConversationTarget, detail::codexErrorInfoCodecDescriptors(), conversationDescriptors),
                                    {detail::ProtocolSurfaceErrorCode::TypedWithoutRuntimeTarget,
                                     detail::ProtocolSurfaceErrorCode::MissingRuntimeTargetRegistration,
                                     detail::ProtocolSurfaceErrorCode::CodecDescriptorTargetMismatch,
                                     detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                                     detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor}),
                      "a missing conversation target fails with the exact bidirectional target/descriptor codes");

    std::vector<detail::ProtocolSurfaceEntry> duplicateConversationTarget = conversationRegistry;
    const auto duplicateTargetRow = findExactEntry(duplicateConversationTarget, conversationDescriptors[1].key);
    duplicateTargetRow->runtimeTarget = conversationDescriptors.front().target;
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(
                                        duplicateConversationTarget, detail::codexErrorInfoCodecDescriptors(), conversationDescriptors),
                                    {detail::ProtocolSurfaceErrorCode::DuplicateRuntimeTargetRegistration,
                                     detail::ProtocolSurfaceErrorCode::MissingRuntimeTargetRegistration,
                                     detail::ProtocolSurfaceErrorCode::CodecDescriptorTargetMismatch,
                                     detail::ProtocolSurfaceErrorCode::RuntimeTargetCanonicalKeyMismatch,
                                     detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                                     detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor}),
                      "a duplicate conversation target fails with the exact duplicate/missing descriptor multiset");

    const std::array<std::pair<std::string_view, detail::ProtocolSurfaceKey>, 3> wrongKeyMutations{{
        {"category",
         {detail::SurfaceCategory::ItemDiscriminator,
          conversationDescriptors.front().key.domain,
          conversationDescriptors.front().key.field,
          conversationDescriptors.front().key.name}},
        {"domain",
         {conversationDescriptors.front().key.category,
          "WrongConversationDomain",
          conversationDescriptors.front().key.field,
          conversationDescriptors.front().key.name}},
        {"field",
         {conversationDescriptors.front().key.category,
          conversationDescriptors.front().key.domain,
          "wrongField",
          conversationDescriptors.front().key.name}},
    }};
    for (const auto& mutation : wrongKeyMutations) {
        std::vector<detail::ProtocolSurfaceEntry> wrongConversationKey = conversationRegistry;
        const auto wrongKeyRow = findExactEntry(wrongConversationKey, conversationDescriptors.front().key);
        wrongKeyRow->key = mutation.second;
        std::sort(wrongConversationKey.begin(), wrongConversationKey.end(), [](const auto& left, const auto& right) {
            return left.key < right.key;
        });
        const auto validationWithWrongKey =
            detail::validateProtocolSurface(wrongConversationKey, detail::codexErrorInfoCodecDescriptors(), conversationDescriptors);
        const bool expectedCodes = mutation.first == std::string_view{"category"}
                                       ? hasExactCodes(validationWithWrongKey,
                                                       {detail::ProtocolSurfaceErrorCode::WrongRuntimeTargetCategory,
                                                        detail::ProtocolSurfaceErrorCode::CodecDescriptorWithoutRegistryRow,
                                                        detail::ProtocolSurfaceErrorCode::RuntimeTargetCanonicalKeyMismatch,
                                                        detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                                                        detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor})
                                       : hasExactCodes(validationWithWrongKey,
                                                       {detail::ProtocolSurfaceErrorCode::CodecDescriptorWithoutRegistryRow,
                                                        detail::ProtocolSurfaceErrorCode::RuntimeTargetCanonicalKeyMismatch,
                                                        detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                                                        detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor});
        result.expectTrue(expectedCodes,
                          std::string("a wrong conversation descriptor ") + std::string(mutation.first) +
                              " fails with its exact canonical-key diagnostic multiset");
    }

    for (const auto& mutation : wrongKeyMutations) {
        std::vector<detail::ProtocolSurfaceEntry> coherentWrongConversationKey = conversationRegistry;
        std::vector<detail::ConversationUnionCodecDescriptor> coherentWrongConversationDescriptor(conversationDescriptors.begin(),
                                                                                                  conversationDescriptors.end());
        const auto wrongKeyRow = findExactEntry(coherentWrongConversationKey, conversationDescriptors.front().key);
        wrongKeyRow->key = mutation.second;
        coherentWrongConversationDescriptor.front().key = mutation.second;
        std::sort(coherentWrongConversationKey.begin(), coherentWrongConversationKey.end(), [](const auto& left, const auto& right) {
            return left.key < right.key;
        });
        const auto validationWithCoherentWrongKey = detail::validateProtocolSurface(
            coherentWrongConversationKey, detail::codexErrorInfoCodecDescriptors(), coherentWrongConversationDescriptor);
        const bool expectedCodes = mutation.first == std::string_view{"category"}
                                       ? hasExactCodes(validationWithCoherentWrongKey,
                                                       {detail::ProtocolSurfaceErrorCode::WrongRuntimeTargetCategory,
                                                        detail::ProtocolSurfaceErrorCode::CodecDescriptorCanonicalKeyMismatch,
                                                        detail::ProtocolSurfaceErrorCode::RuntimeTargetCanonicalKeyMismatch})
                                   : mutation.first == std::string_view{"field"}
                                       ? hasExactCodes(validationWithCoherentWrongKey,
                                                       {detail::ProtocolSurfaceErrorCode::CodecDescriptorCanonicalKeyMismatch,
                                                        detail::ProtocolSurfaceErrorCode::RuntimeTargetCanonicalKeyMismatch,
                                                        detail::ProtocolSurfaceErrorCode::InvalidCodecDescriptorShape})
                                       : hasExactCodes(validationWithCoherentWrongKey,
                                                       {detail::ProtocolSurfaceErrorCode::CodecDescriptorCanonicalKeyMismatch,
                                                        detail::ProtocolSurfaceErrorCode::RuntimeTargetCanonicalKeyMismatch});
        result.expectTrue(expectedCodes,
                          std::string("a coherent wrong conversation row/descriptor ") + std::string(mutation.first) +
                              " mutation cannot evade the exact generated canonical-key guard");
    }

    std::vector<detail::ProtocolSurfaceEntry> falseConversationCompleteness = conversationRegistry;
    const auto falseCompleteRow = findExactEntry(falseConversationCompleteness, conversationDescriptors.front().key);
    falseCompleteRow->schemaCompleteness.directionAssertionsExercised = false;
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(
                                        falseConversationCompleteness, detail::codexErrorInfoCodecDescriptors(), conversationDescriptors),
                                    {detail::ProtocolSurfaceErrorCode::DirectionAssertionMissing}),
                      "a false conversation completeness claim fails with exactly DirectionAssertionMissing");

    std::vector<detail::ProtocolSurfaceEntry> removedConversationRow = conversationRegistry;
    const auto removedConversation = findExactEntry(removedConversationRow, conversationDescriptors.front().key);
    removedConversationRow.erase(removedConversation);
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(
                                        removedConversationRow, detail::codexErrorInfoCodecDescriptors(), conversationDescriptors),
                                    {detail::ProtocolSurfaceErrorCode::MissingRuntimeTargetRegistration,
                                     detail::ProtocolSurfaceErrorCode::CodecDescriptorWithoutRegistryRow}),
                      "removing a conversation registry row while retaining its target descriptor fails with the exact two codes");

    const std::span<const detail::ThreadItemCodecDescriptor> threadItemDescriptors = detail::threadItemCodecDescriptors();
    const std::span<const detail::ResponseItemCodecDescriptor> responseItemDescriptors = detail::responseItemCodecDescriptors();
    result.expectTrue(threadItemDescriptors.size() == 18 &&
                          threadItemDescriptors.size() == static_cast<std::size_t>(detail::ItemDiscriminatorTarget::Count),
                      "the generated ThreadItem descriptor family contains exactly 18 distinct targets");
    result.expectTrue(responseItemDescriptors.size() == 16 &&
                          responseItemDescriptors.size() == static_cast<std::size_t>(detail::ResponseItemTarget::Count),
                      "the generated ResponseItem descriptor family contains exactly 16 distinct targets");

    auto verifyItemDescriptorFamily =
        [&]<typename Descriptor, typename Target>(std::span<const Descriptor> descriptors,
                                                  std::string_view expectedDomain,
                                                  auto validateWithDescriptors) {
            bool exactTargets = true;
            for (std::size_t index = 0; exactTargets && index < descriptors.size(); ++index) {
                const Target target = static_cast<Target>(index);
                exactTargets = descriptors[index].target == target && descriptors[index].key.domain == expectedDomain &&
                               descriptors[index].key.category == detail::SurfaceCategory::ItemDiscriminator &&
                               descriptors[index].key.field == "type" &&
                               descriptors[index].shape == detail::ConversationUnionCodecShape::InternallyTaggedObject &&
                               descriptors[index].direction == detail::ConversationUnionCodecDirection::DecodeOnly &&
                               detail::entryFor(target).key == descriptors[index].key;
            }
            result.expectTrue(exactTargets,
                              std::string(expectedDomain) +
                                  " targets, exact keys, shape, and direction are bidirectionally canonical");

            std::vector<Descriptor> duplicate(descriptors.begin(), descriptors.end());
            duplicate.push_back(descriptors.front());
            result.expectTrue(
                hasExactCodes(validateWithDescriptors(registry, duplicate),
                              {detail::ProtocolSurfaceErrorCode::DuplicateCodecDescriptor}),
                std::string(expectedDomain) + " duplicate descriptor emits exactly DuplicateCodecDescriptor");

            std::vector<Descriptor> missing(descriptors.begin(), descriptors.end());
            missing.erase(missing.begin());
            result.expectTrue(
                hasExactCodes(validateWithDescriptors(registry, missing),
                              {detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                               detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor}),
                std::string(expectedDomain) + " typed complete row without descriptor emits the exact two codes");

            std::vector<Descriptor> stale(descriptors.begin(), descriptors.end());
            stale.front().key.name = "stale_item_descriptor";
            result.expectTrue(
                hasExactCodes(validateWithDescriptors(registry, stale),
                              {detail::ProtocolSurfaceErrorCode::CodecDescriptorCanonicalKeyMismatch,
                               detail::ProtocolSurfaceErrorCode::CodecDescriptorWithoutRegistryRow,
                               detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                               detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor}),
                std::string(expectedDomain) + " stale descriptor emits the exact canonical-key multiset");

            std::vector<Descriptor> swapped(descriptors.begin(), descriptors.end());
            std::swap(swapped[0].target, swapped[1].target);
            result.expectTrue(
                hasExactCodes(validateWithDescriptors(registry, swapped),
                              {detail::ProtocolSurfaceErrorCode::CodecDescriptorCanonicalKeyMismatch,
                               detail::ProtocolSurfaceErrorCode::CodecDescriptorCanonicalKeyMismatch,
                               detail::ProtocolSurfaceErrorCode::CodecDescriptorTargetMismatch,
                               detail::ProtocolSurfaceErrorCode::CodecDescriptorTargetMismatch,
                               detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                               detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                               detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor,
                               detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor}),
                std::string(expectedDomain) + " descriptor target swap emits only the exact eight intrinsic codes");

            std::vector<detail::ProtocolSurfaceEntry> demoted(registry.begin(), registry.end());
            const auto demotedRow = findExactEntry(demoted, descriptors.front().key);
            demotedRow->runtimeDisposition = detail::RuntimeDisposition::OpaquePreserved;
            demotedRow->typedImplementation = detail::TypedImplementationStatus::NotImplemented;
            demotedRow->typedSchemaStatus = detail::TypedSchemaStatus::NotImplemented;
            demotedRow->schemaCompleteness = {};
            result.expectTrue(
                hasExactCodes(validateWithDescriptors(demoted, descriptors),
                              {detail::ProtocolSurfaceErrorCode::RuntimeTargetWithoutTypedImplementation,
                               detail::ProtocolSurfaceErrorCode::CodecDescriptorWithoutTypedRegistryRow}),
                std::string(expectedDomain) + " retained target on a demoted row emits the exact two codes");

            std::vector<detail::ProtocolSurfaceEntry> targetless(registry.begin(), registry.end());
            findExactEntry(targetless, descriptors.front().key)->runtimeTarget = std::monostate{};
            result.expectTrue(
                hasExactCodes(validateWithDescriptors(targetless, descriptors),
                              {detail::ProtocolSurfaceErrorCode::TypedWithoutRuntimeTarget,
                               detail::ProtocolSurfaceErrorCode::MissingRuntimeTargetRegistration,
                               detail::ProtocolSurfaceErrorCode::CodecDescriptorTargetMismatch,
                               detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                               detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor}),
                std::string(expectedDomain) + " missing target emits the exact bidirectional five-code multiset");

            std::vector<detail::ProtocolSurfaceEntry> duplicateTarget(registry.begin(), registry.end());
            findExactEntry(duplicateTarget, descriptors[1].key)->runtimeTarget = descriptors.front().target;
            result.expectTrue(
                hasExactCodes(validateWithDescriptors(duplicateTarget, descriptors),
                              {detail::ProtocolSurfaceErrorCode::DuplicateRuntimeTargetRegistration,
                               detail::ProtocolSurfaceErrorCode::MissingRuntimeTargetRegistration,
                               detail::ProtocolSurfaceErrorCode::CodecDescriptorTargetMismatch,
                               detail::ProtocolSurfaceErrorCode::RuntimeTargetCanonicalKeyMismatch,
                               detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                               detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor}),
                std::string(expectedDomain) + " duplicate target emits the exact duplicate/missing descriptor multiset");

            const std::array<std::pair<std::string_view, detail::ProtocolSurfaceKey>, 3> wrongKeyMutations{{
                {"category",
                 {detail::SurfaceCategory::TaggedUnionDiscriminator,
                  descriptors.front().key.domain,
                  descriptors.front().key.field,
                  descriptors.front().key.name}},
                {"domain",
                 {descriptors.front().key.category,
                  expectedDomain == "ThreadItem" ? "WrongThreadItem" : "WrongResponseItem",
                  descriptors.front().key.field,
                  descriptors.front().key.name}},
                {"field",
                 {descriptors.front().key.category,
                  descriptors.front().key.domain,
                  "wrongItemField",
                  descriptors.front().key.name}},
            }};

            for (const auto& mutation : wrongKeyMutations) {
                std::vector<Descriptor> wrongDescriptor(descriptors.begin(), descriptors.end());
                wrongDescriptor.front().key = mutation.second;
                const auto validationWithWrongDescriptor = validateWithDescriptors(registry, wrongDescriptor);
                const bool expectedCodes =
                    mutation.first == std::string_view{"field"}
                        ? hasExactCodes(validationWithWrongDescriptor,
                                        {detail::ProtocolSurfaceErrorCode::CodecDescriptorCanonicalKeyMismatch,
                                         detail::ProtocolSurfaceErrorCode::CodecDescriptorWithoutRegistryRow,
                                         detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                                         detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor,
                                         detail::ProtocolSurfaceErrorCode::InvalidCodecDescriptorShape})
                        : hasExactCodes(validationWithWrongDescriptor,
                                        {detail::ProtocolSurfaceErrorCode::CodecDescriptorCanonicalKeyMismatch,
                                         detail::ProtocolSurfaceErrorCode::CodecDescriptorWithoutRegistryRow,
                                         detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                                         detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor});
                result.expectTrue(expectedCodes,
                                  std::string(expectedDomain) + " wrong descriptor " + std::string(mutation.first) +
                                      " emits only its exact bidirectional key-association codes");
            }

            for (const auto& mutation : wrongKeyMutations) {
                std::vector<detail::ProtocolSurfaceEntry> wrongRegistryKey(registry.begin(), registry.end());
                findExactEntry(wrongRegistryKey, descriptors.front().key)->key = mutation.second;
                std::sort(wrongRegistryKey.begin(), wrongRegistryKey.end(), [](const auto& left, const auto& right) {
                    return left.key < right.key;
                });
                const auto validationWithWrongRegistryKey = validateWithDescriptors(wrongRegistryKey, descriptors);
                const bool expectedCodes =
                    mutation.first == std::string_view{"category"}
                        ? hasExactCodes(validationWithWrongRegistryKey,
                                        {detail::ProtocolSurfaceErrorCode::WrongRuntimeTargetCategory,
                                         detail::ProtocolSurfaceErrorCode::CodecDescriptorWithoutRegistryRow,
                                         detail::ProtocolSurfaceErrorCode::RuntimeTargetCanonicalKeyMismatch,
                                         detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                                         detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor})
                        : hasExactCodes(validationWithWrongRegistryKey,
                                        {detail::ProtocolSurfaceErrorCode::CodecDescriptorWithoutRegistryRow,
                                         detail::ProtocolSurfaceErrorCode::RuntimeTargetCanonicalKeyMismatch,
                                         detail::ProtocolSurfaceErrorCode::RegistryRowWithoutCodecDescriptor,
                                         detail::ProtocolSurfaceErrorCode::CompleteWithoutCodecDescriptor});
                result.expectTrue(expectedCodes,
                                  std::string(expectedDomain) + " wrong registry " + std::string(mutation.first) +
                                      " emits only its exact bidirectional key-association codes");
            }

            for (const auto& mutation : wrongKeyMutations) {
                std::vector<detail::ProtocolSurfaceEntry> coherentWrongKey(registry.begin(), registry.end());
                std::vector<Descriptor> coherentWrongDescriptor(descriptors.begin(), descriptors.end());
                findExactEntry(coherentWrongKey, descriptors.front().key)->key = mutation.second;
                coherentWrongDescriptor.front().key = mutation.second;
                std::sort(coherentWrongKey.begin(), coherentWrongKey.end(), [](const auto& left, const auto& right) {
                    return left.key < right.key;
                });
                const auto validationWithCoherentWrongKey =
                    validateWithDescriptors(coherentWrongKey, coherentWrongDescriptor);
                const bool expectedCodes =
                    mutation.first == std::string_view{"category"}
                        ? hasExactCodes(validationWithCoherentWrongKey,
                                        {detail::ProtocolSurfaceErrorCode::WrongRuntimeTargetCategory,
                                         detail::ProtocolSurfaceErrorCode::CodecDescriptorCanonicalKeyMismatch,
                                         detail::ProtocolSurfaceErrorCode::RuntimeTargetCanonicalKeyMismatch})
                    : mutation.first == std::string_view{"field"}
                        ? hasExactCodes(validationWithCoherentWrongKey,
                                        {detail::ProtocolSurfaceErrorCode::CodecDescriptorCanonicalKeyMismatch,
                                         detail::ProtocolSurfaceErrorCode::RuntimeTargetCanonicalKeyMismatch,
                                         detail::ProtocolSurfaceErrorCode::InvalidCodecDescriptorShape})
                        : hasExactCodes(validationWithCoherentWrongKey,
                                        {detail::ProtocolSurfaceErrorCode::CodecDescriptorCanonicalKeyMismatch,
                                         detail::ProtocolSurfaceErrorCode::RuntimeTargetCanonicalKeyMismatch});
                result.expectTrue(expectedCodes,
                                  std::string(expectedDomain) + " coherent wrong row/descriptor " +
                                      std::string(mutation.first) +
                                      " cannot evade the exact generated canonical-key guard");
            }

            std::vector<detail::ProtocolSurfaceEntry> falseComplete(registry.begin(), registry.end());
            findExactEntry(falseComplete, descriptors.front().key)->schemaCompleteness.noKnownSchemaFieldsDropped = false;
            result.expectTrue(
                hasExactCodes(validateWithDescriptors(falseComplete, descriptors),
                              {detail::ProtocolSurfaceErrorCode::KnownSchemaFieldDropped}),
                std::string(expectedDomain) + " false completeness emits exactly KnownSchemaFieldDropped");

            std::vector<detail::ProtocolSurfaceEntry> removed(registry.begin(), registry.end());
            removed.erase(findExactEntry(removed, descriptors.front().key));
            result.expectTrue(
                hasExactCodes(validateWithDescriptors(removed, descriptors),
                              {detail::ProtocolSurfaceErrorCode::MissingRuntimeTargetRegistration,
                               detail::ProtocolSurfaceErrorCode::CodecDescriptorWithoutRegistryRow}),
                std::string(expectedDomain) + " removed row with retained descriptor emits exactly two codes");
        };

    verifyItemDescriptorFamily.template operator()<detail::ThreadItemCodecDescriptor, detail::ItemDiscriminatorTarget>(
        threadItemDescriptors,
        "ThreadItem",
        [&](std::span<const detail::ProtocolSurfaceEntry> entries,
            std::span<const detail::ThreadItemCodecDescriptor> descriptors) {
            return detail::validateProtocolSurface(entries,
                                                   detail::codexErrorInfoCodecDescriptors(),
                                                   conversationDescriptors,
                                                   descriptors,
                                                   responseItemDescriptors);
        });
    verifyItemDescriptorFamily.template operator()<detail::ResponseItemCodecDescriptor, detail::ResponseItemTarget>(
        responseItemDescriptors,
        "ResponseItem",
        [&](std::span<const detail::ProtocolSurfaceEntry> entries,
            std::span<const detail::ResponseItemCodecDescriptor> descriptors) {
            return detail::validateProtocolSurface(entries,
                                                   detail::codexErrorInfoCodecDescriptors(),
                                                   conversationDescriptors,
                                                   threadItemDescriptors,
                                                   descriptors);
        });

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
    result.expectTrue(unknownThreadItem && unknownThreadItem->runtimeDisposition == detail::RuntimeDisposition::Typed &&
                          unknownThreadItem->typedSchemaStatus == detail::TypedSchemaStatus::Complete &&
                          unknownThreadItem->frontendProtocol == detail::FrontendExposure::ExistingUnknownItemSubset &&
                          unknownThreadItem->frontendSecurity == detail::FrontendSecurityDecision::ExistingUnknownItemMetadataContract,
                      "newly typed ThreadItem rows preserve only the existing metadata-only frontend subset");

    const detail::ProtocolSurfaceEntry* responseItem =
        detail::findSurface(detail::SurfaceCategory::ItemDiscriminator, "ResponseItem", "type", "message");
    result.expectTrue(responseItem && responseItem->runtimeDisposition == detail::RuntimeDisposition::Typed &&
                          responseItem->typedSchemaStatus == detail::TypedSchemaStatus::Complete &&
                          responseItem->frontendProtocol == detail::FrontendExposure::NotExposed &&
                          responseItem->frontendSecurity == detail::FrontendSecurityDecision::Unresolved,
                      "typed ResponseItem rows remain distinct and do not claim a frontend payload path");

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
