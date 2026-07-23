/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

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
    std::array<std::size_t, 7> categories{};
    for (const detail::ProtocolSurfaceEntry& entry : registry) {
        entry.stability == detail::Stability::Stable ? ++stable : ++experimentalOnly;
        ++categories[static_cast<std::size_t>(entry.key.category)];
        if (entry.stability == detail::Stability::Stable) {
            normalizedFrontendEvents += entry.frontendProtocol == detail::FrontendExposure::ExistingEventSubset;
            genericFrontendExtensions += entry.frontendProtocol == detail::FrontendExposure::GenericExtension;
            unknownItemMetadataSubsets += entry.frontendProtocol == detail::FrontendExposure::ExistingUnknownItemSubset;
            responseItemsNotExposed += entry.key.category == detail::SurfaceCategory::ItemDiscriminator &&
                                       entry.key.domain == "ResponseItem" && entry.frontendProtocol == detail::FrontendExposure::NotExposed;
        }
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

    return result.processResult();
}
