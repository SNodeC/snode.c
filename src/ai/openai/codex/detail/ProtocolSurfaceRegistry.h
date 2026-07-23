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

    enum class ResultContractKind { Concrete, Unit, Nullable, ProtocolSpecial, Unresolved, NotApplicable };

    enum class AssociationEvidenceKind { VendoredRust, VendoredSchemaPair, VendoredTypeScriptCrossCheck, NotApplicable };

    enum class A1Slice { A1_0, A1_1, A1_2, A1_3, A1_4, InventoryOnly, Unassigned };

    enum class TypedSchemaStatus { Complete, Partial, NotImplemented, NotApplicable };

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

    enum class CodexErrorInfoTarget {
        ActiveTurnNotSteerable,
        BadRequest,
        ContextWindowExceeded,
        CyberPolicy,
        HttpConnectionFailed,
        InternalServerError,
        Other,
        ResponseStreamConnectionFailed,
        ResponseStreamDisconnected,
        ResponseTooManyFailedAttempts,
        SandboxError,
        ServerOverloaded,
        SessionBudgetExceeded,
        ThreadRollbackFailed,
        Unauthorized,
        UsageLimitExceeded,
        Count
    };

    enum class CodexErrorInfoCodecShape {
        Scalar,
        HttpStatusObject,
        ActiveTurnNotSteerableObject
    };

    using RuntimeTarget = std::variant<std::monostate,
                                       ClientRequestTarget,
                                       ClientNotificationTarget,
                                       ServerNotificationTarget,
                                       ServerRequestTarget,
                                       ItemDiscriminatorTarget,
                                       CodexErrorInfoTarget>;

    struct ProtocolSurfaceKey {
        SurfaceCategory category = SurfaceCategory::TaggedUnionDiscriminator;
        std::string_view domain;
        std::string_view field;
        std::string_view name;

        auto operator<=>(const ProtocolSurfaceKey&) const = default;
    };

    struct CodexErrorInfoCodecDescriptor {
        ProtocolSurfaceKey key;
        CodexErrorInfoTarget target = CodexErrorInfoTarget::Count;
        CodexErrorInfoCodecShape shape = CodexErrorInfoCodecShape::Scalar;
    };

    struct OperationContract {
        std::string_view parameterTypeIdentity;
        std::string_view resultTypeIdentity;
        ResultContractKind resultKind = ResultContractKind::NotApplicable;
        AssociationEvidenceKind evidenceKind = AssociationEvidenceKind::NotApplicable;
        std::string_view evidenceKey;
    };

    struct SchemaCompletenessEvidence {
        bool authoritativeRootAssociation = false;
        bool positiveFixtureCoverage = false;
        bool requiredFieldsExercised = false;
        bool schemaPropertiesExercised = false;
        bool optionalPresentExercised = false;
        bool optionalOmittedExercised = false;
        bool nullableSemanticsExercised = false;
        bool reachableUnionAlternativesExercised = false;
        bool directionAssertionsExercised = false;
        bool fixtureCurrent = false;
        bool runtimeDecoderMatchesRegistry = false;
        bool opaqueFieldsDeclared = false;
        bool independentlySchemaValidated = false;
        bool noKnownSchemaFieldsDropped = false;
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
        OperationContract operationContract;
        std::string_view typedModule;
        A1Slice a1Slice = A1Slice::Unassigned;
        TypedSchemaStatus typedSchemaStatus = TypedSchemaStatus::NotImplemented;
        SchemaCompletenessEvidence schemaCompleteness;
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
        DuplicateCodecDescriptor,
        CodecDescriptorWithoutRegistryRow,
        CodecDescriptorTargetMismatch,
        CodecDescriptorWithoutTypedRegistryRow,
        RegistryRowWithoutCodecDescriptor,
        CompleteWithoutCodecDescriptor,
        DuplicateManifestEntry,
        MissingRegistryEntry,
        WrongCategory,
        WrongStability,
        WrongDeprecation,
        StaleRegistryEntry,
        MissingAssociation,
        DuplicateAssociation,
        StaleAssociation,
        WrongAssociationCategory,
        WrongParameterType,
        WrongResultType,
        ConflictingAssociationEvidence,
        UnitWithNonUnitResultType,
        ConcreteWithoutResultType,
        ContractOnNonRequest,
        ExperimentalAssociationCountedAsStable,
        MissingTypedModuleAssignment,
        MissingSliceAssignment,
        RequiredFieldNotExercised,
        SchemaPropertyNotExercised,
        OptionalPresentCaseMissing,
        OptionalOmittedCaseMissing,
        NullableSemanticsMissing,
        ReachableUnionAlternativeMissing,
        DirectionAssertionMissing,
        StaleFixture,
        CompletenessRuntimeTargetMismatch,
        UnrecordedOpaqueField,
        ClaimedCompleteWithoutAuthoritativeAssociation,
        ClaimedCompleteWithoutPositiveFixtureCoverage,
        ClaimedCompleteWithoutIndependentValidation,
        KnownSchemaFieldDropped,
        TypedSchemaStatusMismatch
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
    const ProtocolSurfaceEntry& entryFor(CodexErrorInfoTarget target);

    TypedSchemaStatus derivedTypedSchemaStatus(const ProtocolSurfaceEntry& entry) noexcept;

    ProtocolSurfaceValidation validateProtocolSurface(std::span<const ProtocolSurfaceEntry> entries);
    ProtocolSurfaceValidation validateProtocolSurface(
        std::span<const ProtocolSurfaceEntry> entries,
        std::span<const CodexErrorInfoCodecDescriptor> codexErrorInfoDescriptors);

} // namespace ai::openai::codex::detail

#endif // AI_OPENAI_CODEX_DETAIL_PROTOCOLSURFACEREGISTRY_H
