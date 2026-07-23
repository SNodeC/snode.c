/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef AI_OPENAI_CODEX_DETAIL_PROTOCOLSURFACEREGISTRY_H
#define AI_OPENAI_CODEX_DETAIL_PROTOCOLSURFACEREGISTRY_H

#include <compare>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace ai::openai::codex::detail {

    enum class SurfaceCategory {
        ClientNotification,
        ClientRequest,
        DeltaProgressDiscriminator,
        ItemDiscriminator,
        ServerNotification,
        ServerRequest,
        TaggedUnionDiscriminator
    };

    enum class Stability { Stable, ExperimentalOnly };

    enum class RuntimeDisposition { Typed, RawPreserved, OpaquePreserved, Unsupported, Deferred };

    enum class TypedImplementationStatus { Implemented, NotImplemented, NotApplicable };

    enum class LayerStatus { Implemented, NotImplemented, NotApplicable };

    enum class FrontendExposure {
        ExistingOperationSubset,
        GenericUnknownRequest,
        ExistingEventSubset,
        GenericExtension,
        ExistingUnknownItemSubset,
        NotExposed,
        NotApplicable
    };

    enum class FrontendSecurityDecision {
        ExistingOperationSubsetExpansionUnresolved,
        ExistingGenericContractDedicatedUnresolved,
        ExistingEventSubsetContract,
        ExistingRedactedExtensionContract,
        ExistingUnknownItemMetadataContract,
        Unresolved,
        NotApplicable
    };

    enum class ClientRequestTarget { Initialize, ThreadStart, ThreadResume, ThreadList, ThreadRead, TurnStart, TurnInterrupt, Count };

    enum class ClientNotificationTarget { Initialized, Count };

    enum class ServerNotificationTarget {
        Error,
        ThreadStarted,
        ThreadStatusChanged,
        TurnStarted,
        TurnCompleted,
        ItemStarted,
        ItemCompleted,
        AgentMessageDelta,
        ReasoningTextDelta,
        ReasoningSummaryTextDelta,
        CommandExecutionOutputDelta,
        FileChangePatchUpdated,
        ThreadTokenUsageUpdated,
        ModelRerouted,
        Count
    };

    enum class ServerRequestTarget {
        CommandExecutionRequestApproval,
        FileChangeRequestApproval,
        ToolRequestUserInput,
        ChatgptAuthTokensRefresh,
        Count
    };

    enum class ItemDiscriminatorTarget {
        AgentMessage,
        UserMessage,
        Reasoning,
        CommandExecution,
        FileChange,
        McpToolCall,
        DynamicToolCall,
        WebSearch,
        Count
    };

    using RuntimeTarget = std::variant<std::monostate,
                                       ClientRequestTarget,
                                       ClientNotificationTarget,
                                       ServerNotificationTarget,
                                       ServerRequestTarget,
                                       ItemDiscriminatorTarget>;

    struct ProtocolSurfaceKey {
        SurfaceCategory category = SurfaceCategory::TaggedUnionDiscriminator;
        std::string_view domain;
        std::string_view field;
        std::string_view name;

        auto operator<=>(const ProtocolSurfaceKey&) const = default;
    };

    struct ProtocolSurfaceEntry {
        ProtocolSurfaceKey key;
        Stability stability = Stability::Stable;
        bool deprecated = false;
        RuntimeDisposition runtimeDisposition = RuntimeDisposition::Deferred;
        TypedImplementationStatus typedImplementation = TypedImplementationStatus::NotImplemented;
        LayerStatus backendCore = LayerStatus::NotImplemented;
        LayerStatus canonicalState = LayerStatus::NotImplemented;
        FrontendExposure frontendProtocol = FrontendExposure::NotExposed;
        FrontendSecurityDecision frontendSecurity = FrontendSecurityDecision::Unresolved;
        RuntimeTarget runtimeTarget;
    };

    enum class ProtocolSurfaceErrorCode {
        EmptyName,
        EmptyDiscriminatorField,
        InvalidRuntimeTarget,
        WrongRuntimeTargetCategory,
        TypedWithoutRuntimeTarget,
        RuntimeTargetWithoutTypedImplementation,
        TypedDispositionWithoutImplementation,
        ImplementedWithoutTypedDisposition,
        FrontendSecurityMismatch,
        UnsortedRegistry,
        DuplicateRegistryEntry,
        DiscriminatorCategoryCollision,
        MethodCategoryCollision,
        DuplicateRuntimeTargetRegistration,
        MissingRuntimeTargetRegistration,
        DuplicateManifestEntry,
        MissingRegistryEntry,
        WrongCategory,
        WrongStability,
        WrongDeprecation,
        StaleRegistryEntry
    };

    struct ProtocolSurfaceDiagnostic {
        ProtocolSurfaceErrorCode code;
        std::string message;
    };

    struct ProtocolSurfaceValidation {
        std::vector<ProtocolSurfaceDiagnostic> errors;

        explicit operator bool() const noexcept {
            return errors.empty();
        }
    };

    std::span<const ProtocolSurfaceEntry> protocolSurfaceRegistry() noexcept;

    const ProtocolSurfaceEntry*
    findSurface(SurfaceCategory category, std::string_view domain, std::string_view field, std::string_view name) noexcept;

    const ProtocolSurfaceEntry& entryFor(ClientRequestTarget target);
    const ProtocolSurfaceEntry& entryFor(ClientNotificationTarget target);
    const ProtocolSurfaceEntry& entryFor(ServerNotificationTarget target);
    const ProtocolSurfaceEntry& entryFor(ServerRequestTarget target);
    const ProtocolSurfaceEntry& entryFor(ItemDiscriminatorTarget target);

    ProtocolSurfaceValidation validateProtocolSurface(std::span<const ProtocolSurfaceEntry> entries);

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_PROTOCOLSURFACEREGISTRY_H
