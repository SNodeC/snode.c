/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "component/codex/CodexA11B2TypedSurfaceBaseline.h"
#include "component/codex/CodexErrorInfoTypedSurfaceBaseline.h"
#include "component/codex/CodexTypedSurfaceBaseline.h"
#include "support/TestResult.h"

#include <algorithm>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <map>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#ifndef CODEX_PHASE_A0_SURFACE_MANIFEST
#error "CODEX_PHASE_A0_SURFACE_MANIFEST must name the pinned surface manifest"
#endif

namespace {
    namespace detail = ai::openai::codex::detail;
    using ai::openai::codex::Json;

    using OwnedKey = std::tuple<detail::SurfaceCategory, std::string, std::string, std::string>;
    using ErrorCode = detail::ProtocolSurfaceErrorCode;

    struct ExpectedEntry {
        detail::Stability stability;
        bool deprecated;
    };

    enum class TypedCoverageRatchetErrorCode {
        BaselineIdentityMissing,
        BaselineIdentityNotStable,
        BaselineIdentityNotImplemented,
        B2IdentityWrongAssignment,
        B2IdentityNotComplete,
        B4IdentityMissing,
        B4IdentityWrongAssignment,
        B4IdentityNotComplete
    };

    struct TypedCoverageRatchetDiagnostic {
        TypedCoverageRatchetErrorCode code;
        std::string message;
    };

    detail::SurfaceCategory categoryFromJson(const std::string& category) {
        static const std::map<std::string, detail::SurfaceCategory> Categories{
            {"client_notification", detail::SurfaceCategory::ClientNotification},
            {"client_request", detail::SurfaceCategory::ClientRequest},
            {"delta_progress_discriminator", detail::SurfaceCategory::DeltaProgressDiscriminator},
            {"item_discriminator", detail::SurfaceCategory::ItemDiscriminator},
            {"server_notification", detail::SurfaceCategory::ServerNotification},
            {"server_request", detail::SurfaceCategory::ServerRequest},
            {"tagged_union_discriminator", detail::SurfaceCategory::TaggedUnionDiscriminator},
        };
        return Categories.at(category);
    }

    OwnedKey ownedKey(const detail::ProtocolSurfaceEntry& entry) {
        return {entry.key.category, std::string(entry.key.domain), std::string(entry.key.field), std::string(entry.key.name)};
    }

    OwnedKey expectedKey(const Json& entry) {
        return {categoryFromJson(entry.at("category").get<std::string>()),
                entry.at("domain").get<std::string>(),
                entry.at("discriminator_field").get<std::string>(),
                entry.at("name").get<std::string>()};
    }

    template <typename Diagnostic, typename Code>
    bool hasExactCodes(std::span<const Diagnostic> diagnostics, std::initializer_list<Code> expected) {
        std::vector<Code> actualCodes;
        actualCodes.reserve(diagnostics.size());
        std::transform(diagnostics.begin(), diagnostics.end(), std::back_inserter(actualCodes), [](const Diagnostic& diagnostic) {
            return diagnostic.code;
        });
        std::vector<Code> expectedCodes(expected);
        std::sort(actualCodes.begin(), actualCodes.end());
        std::sort(expectedCodes.begin(), expectedCodes.end());
        return actualCodes == expectedCodes;
    }

    bool hasExactCodes(const detail::ProtocolSurfaceValidation& validation, std::initializer_list<ErrorCode> expected) {
        return hasExactCodes<detail::ProtocolSurfaceDiagnostic, ErrorCode>(validation.errors, expected);
    }

    std::vector<detail::ProtocolSurfaceDiagnostic> coverageErrors(std::span<const detail::ProtocolSurfaceEntry> registry,
                                                                  const Json& manifest) {
        std::vector<detail::ProtocolSurfaceDiagnostic> errors = detail::validateProtocolSurface(registry).errors;
        std::map<OwnedKey, ExpectedEntry> expected;
        for (const Json& manifestEntry : manifest.at("entries")) {
            const detail::Stability stability =
                manifestEntry.at("stability") == "stable" ? detail::Stability::Stable : detail::Stability::ExperimentalOnly;
            const auto [iterator, inserted] =
                expected.emplace(expectedKey(manifestEntry), ExpectedEntry{stability, manifestEntry.at("deprecated").get<bool>()});
            (void) iterator;
            if (!inserted) {
                errors.push_back({ErrorCode::DuplicateManifestEntry, "duplicate schema-derived manifest entry"});
            }
        }

        std::map<OwnedKey, const detail::ProtocolSurfaceEntry*> actual;
        for (const detail::ProtocolSurfaceEntry& registryEntry : registry) {
            // validateProtocolSurface reports duplicate identities. Keeping the
            // first row here avoids reporting the same fault a second time.
            actual.emplace(ownedKey(registryEntry), &registryEntry);
        }

        std::set<OwnedKey> wrongCategoryRows;
        for (const auto& [key, expectedEntry] : expected) {
            const auto iterator = actual.find(key);
            if (iterator == actual.end()) {
                const auto& [expectedCategory, expectedDomain, expectedField, expectedName] = key;
                const auto wrongCategory = std::find_if(actual.begin(), actual.end(), [&](const auto& item) {
                    const auto& [actualCategory, actualDomain, actualField, actualName] = item.first;
                    return actualCategory != expectedCategory && actualDomain == expectedDomain && actualField == expectedField &&
                           actualName == expectedName;
                });
                if (wrongCategory != actual.end()) {
                    errors.push_back({ErrorCode::WrongCategory, "registry category differs from schema-derived manifest"});
                    wrongCategoryRows.insert(wrongCategory->first);
                } else {
                    errors.push_back({ErrorCode::MissingRegistryEntry, "schema-derived entry absent from canonical registry"});
                }
                continue;
            }
            if (iterator->second->stability != expectedEntry.stability) {
                errors.push_back({ErrorCode::WrongStability, "registry stability differs from schema-derived manifest"});
            }
            if (iterator->second->deprecated != expectedEntry.deprecated) {
                errors.push_back({ErrorCode::WrongDeprecation, "registry deprecation differs from schema-derived manifest"});
            }
        }
        for (const auto& [key, registryEntry] : actual) {
            (void) registryEntry;
            if (!expected.contains(key) && !wrongCategoryRows.contains(key)) {
                errors.push_back({ErrorCode::StaleRegistryEntry, "stale registry entry is absent from schema-derived manifest"});
            }
        }
        return errors;
    }

    bool hasExactCoverageCodes(const std::vector<detail::ProtocolSurfaceEntry>& entries,
                               const Json& manifest,
                               std::initializer_list<ErrorCode> expected) {
        const std::vector<detail::ProtocolSurfaceDiagnostic> errors =
            coverageErrors(std::span<const detail::ProtocolSurfaceEntry>{entries}, manifest);
        return hasExactCodes<detail::ProtocolSurfaceDiagnostic, ErrorCode>(errors, expected);
    }

    std::vector<TypedCoverageRatchetDiagnostic> typedCoverageRatchetErrors(std::span<const detail::ProtocolSurfaceEntry> registry) {
        std::vector<TypedCoverageRatchetDiagnostic> errors;
        const auto findIdentity = [&](const tests::component::codex::TypedSurfaceIdentity& baselineIdentity) {
            const auto iterator = std::find_if(registry.begin(), registry.end(), [&](const detail::ProtocolSurfaceEntry& entry) {
                return entry.key.category == baselineIdentity.category && entry.key.domain == baselineIdentity.domain &&
                       entry.key.field == baselineIdentity.field && entry.key.name == baselineIdentity.name;
            });
            if (iterator == registry.end()) {
                errors.push_back({TypedCoverageRatchetErrorCode::BaselineIdentityMissing, "locked stable typed identity is absent"});
                return registry.end();
            }
            if (iterator->stability != detail::Stability::Stable) {
                errors.push_back({TypedCoverageRatchetErrorCode::BaselineIdentityNotStable, "locked typed identity is no longer stable"});
            }
            if (iterator->runtimeDisposition != detail::RuntimeDisposition::Typed ||
                iterator->typedImplementation != detail::TypedImplementationStatus::Implemented) {
                errors.push_back({TypedCoverageRatchetErrorCode::BaselineIdentityNotImplemented,
                                  "locked stable typed identity is no longer implemented"});
            }
            return iterator;
        };
        for (const tests::component::codex::TypedSurfaceIdentity& baselineIdentity : tests::component::codex::TypedSurfaceBaseline) {
            (void) findIdentity(baselineIdentity);
        }
        for (const tests::component::codex::TypedSurfaceIdentity& baselineIdentity :
             tests::component::codex::CodexErrorInfoTypedSurfaceBaseline) {
            (void) findIdentity(baselineIdentity);
        }
        for (const tests::component::codex::TypedSurfaceIdentity& baselineIdentity :
             tests::component::codex::CodexA11B2TypedSurfaceBaseline) {
            const auto iterator = findIdentity(baselineIdentity);
            if (iterator == registry.end()) {
                continue;
            }
            if (iterator->a1Slice != detail::A1Slice::A1_1 || iterator->typedModule != "Common") {
                errors.push_back({TypedCoverageRatchetErrorCode::B2IdentityWrongAssignment,
                                  "locked A1.1 B2 identity is no longer assigned to A1.1/Common"});
            }
            if (iterator->typedSchemaStatus != detail::TypedSchemaStatus::Complete ||
                detail::derivedTypedSchemaStatus(*iterator) != detail::TypedSchemaStatus::Complete) {
                errors.push_back(
                    {TypedCoverageRatchetErrorCode::B2IdentityNotComplete, "locked A1.1 B2 identity is not mechanically schema-complete"});
            }
        }
        const auto checkB4 = [&](const detail::ProtocolSurfaceEntry& entry) {
            if (entry.a1Slice != detail::A1Slice::A1_1 ||
                entry.typedModule != "ThreadsTurnsSessions") {
                errors.push_back(
                    {TypedCoverageRatchetErrorCode::
                         B4IdentityWrongAssignment,
                     "locked A1.1 B4 identity is no longer assigned to "
                     "A1.1/ThreadsTurnsSessions"});
            }
            if (
                entry.typedSchemaStatus !=
                    detail::TypedSchemaStatus::Complete ||
                detail::derivedTypedSchemaStatus(entry) !=
                    detail::TypedSchemaStatus::Complete) {
                errors.push_back(
                    {TypedCoverageRatchetErrorCode::
                         B4IdentityNotComplete,
                     "locked A1.1 B4 identity is not mechanically "
                     "schema-complete"});
            }
        };
        for (const auto& descriptor :
             detail::clientOperationCodecDescriptors()) {
            const auto iterator = std::find_if(
                registry.begin(), registry.end(),
                [&](const detail::ProtocolSurfaceEntry& entry) {
                    return entry.key == descriptor.key;
                });
            if (iterator != registry.end()) {
                checkB4(*iterator);
            } else {
                errors.push_back(
                    {TypedCoverageRatchetErrorCode::B4IdentityMissing,
                     "locked A1.1 B4 operation identity is absent"});
            }
        }
        for (const auto& descriptor :
             detail::conversationUnionCodecDescriptors()) {
            if (
                descriptor.key.domain == "SessionSource" ||
                descriptor.key.domain == "SubAgentSource" ||
                descriptor.key.domain == "ThreadStatus") {
                const auto iterator = std::find_if(
                    registry.begin(), registry.end(),
                    [&](const detail::ProtocolSurfaceEntry& entry) {
                        return entry.key == descriptor.key;
                    });
                if (iterator != registry.end()) {
                    checkB4(*iterator);
                } else {
                    errors.push_back(
                        {TypedCoverageRatchetErrorCode::B4IdentityMissing,
                         "locked A1.1 B4 nested-union identity is absent"});
                }
            }
        }
        return errors;
    }

    bool hasExactRatchetCodes(const std::vector<detail::ProtocolSurfaceEntry>& entries,
                              std::initializer_list<TypedCoverageRatchetErrorCode> expected) {
        const std::vector<TypedCoverageRatchetDiagnostic> errors =
            typedCoverageRatchetErrors(std::span<const detail::ProtocolSurfaceEntry>{entries});
        return hasExactCodes<TypedCoverageRatchetDiagnostic, TypedCoverageRatchetErrorCode>(errors, expected);
    }

    auto findEntry(std::vector<detail::ProtocolSurfaceEntry>& entries,
                   detail::SurfaceCategory category,
                   std::string_view domain,
                   std::string_view field,
                   std::string_view name) {
        return std::find_if(entries.begin(), entries.end(), [&](const detail::ProtocolSurfaceEntry& entry) {
            return entry.key.category == category && entry.key.domain == domain && entry.key.field == field && entry.key.name == name;
        });
    }

    std::size_t typedIdentityCount(std::span<const detail::ProtocolSurfaceEntry> entries) {
        return static_cast<std::size_t>(std::count_if(entries.begin(), entries.end(), [](const detail::ProtocolSurfaceEntry& entry) {
            return entry.stability == detail::Stability::Stable && entry.runtimeDisposition == detail::RuntimeDisposition::Typed &&
                   entry.typedImplementation == detail::TypedImplementationStatus::Implemented;
        }));
    }

    struct SchemaStatusCounts {
        std::size_t complete = 0;
        std::size_t partial = 0;
        std::size_t notImplemented = 0;
        std::size_t notApplicable = 0;

        bool operator==(const SchemaStatusCounts&) const = default;
    };

    SchemaStatusCounts schemaStatusCounts(std::span<const detail::ProtocolSurfaceEntry> entries, bool a11Only = false) {
        SchemaStatusCounts counts;
        for (const detail::ProtocolSurfaceEntry& entry : entries) {
            if (a11Only && entry.a1Slice != detail::A1Slice::A1_1) {
                continue;
            }
            switch (entry.typedSchemaStatus) {
                case detail::TypedSchemaStatus::Complete:
                    ++counts.complete;
                    break;
                case detail::TypedSchemaStatus::Partial:
                    ++counts.partial;
                    break;
                case detail::TypedSchemaStatus::NotImplemented:
                    ++counts.notImplemented;
                    break;
                case detail::TypedSchemaStatus::NotApplicable:
                    ++counts.notApplicable;
                    break;
            }
        }
        return counts;
    }

    Json loadManifest() {
        std::ifstream input(CODEX_PHASE_A0_SURFACE_MANIFEST);
        if (!input) {
            throw std::runtime_error("unable to open pinned Codex App Server surface manifest");
        }
        Json manifest;
        input >> manifest;
        return manifest;
    }
} // namespace

int main() {
    tests::support::TestResult result;
    const Json manifest = loadManifest();
    const std::span<const detail::ProtocolSurfaceEntry> production = detail::protocolSurfaceRegistry();
    const std::vector<detail::ProtocolSurfaceEntry> baseline(production.begin(), production.end());

    result.expectTrue(hasExactCoverageCodes(baseline, manifest, {}),
                      "schema-derived manifest and canonical production runtime registry agree exactly");
    result.expectTrue(hasExactRatchetCodes(baseline, {}),
                      "the staged B4 registry retains every locked A1.0, B2, and exact operation-batch identity");
    result.expectTrue(typedIdentityCount(baseline) == tests::component::codex::TypedSurfaceBaseline.size() +
                                                          tests::component::codex::CodexErrorInfoTypedSurfaceBaseline.size() +
                                                          tests::component::codex::CodexA11B2TypedSurfaceBaseline.size() + 74,
                      "B3 and B4 add exactly 74 new typed identities while completing inherited partial rows");
    result.expectTrue(schemaStatusCounts(baseline, true) == SchemaStatusCounts{114, 12, 25, 0},
                      "the staged A1.1 slice is exactly Complete 114, Partial 12, NotImplemented 25, NotApplicable 0");
    result.expectTrue(schemaStatusCounts(baseline) == SchemaStatusCounts{130, 20, 189, 48},
                      "the staged global registry is exactly Complete 130, Partial 20, NotImplemented 189, NotApplicable 48");

    std::vector<detail::ProtocolSurfaceEntry> missing = baseline;
    const auto missingEntry =
        findEntry(missing, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "account/login/cancel");
    missing.erase(missingEntry);
    result.expectTrue(hasExactCoverageCodes(missing, manifest, {ErrorCode::MissingRegistryEntry}),
                      "coverage guard reports only MissingRegistryEntry for removal of one untyped stable schema-derived row");

    std::vector<detail::ProtocolSurfaceEntry> duplicate = baseline;
    duplicate.push_back(
        *findEntry(duplicate, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "account/login/cancel"));
    std::sort(duplicate.begin(), duplicate.end(), [](const auto& left, const auto& right) {
        return left.key < right.key;
    });
    result.expectTrue(hasExactCoverageCodes(duplicate, manifest, {ErrorCode::DuplicateRegistryEntry}),
                      "coverage guard reports only DuplicateRegistryEntry for one duplicated untyped row");

    std::vector<detail::ProtocolSurfaceEntry> wrongCategory = baseline;
    const auto recategorized =
        findEntry(wrongCategory, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "account/login/cancel");
    recategorized->key.category = detail::SurfaceCategory::TaggedUnionDiscriminator;
    recategorized->operationContract = {};
    std::sort(wrongCategory.begin(), wrongCategory.end(), [](const auto& left, const auto& right) {
        return left.key < right.key;
    });
    result.expectTrue(hasExactCoverageCodes(wrongCategory, manifest, {ErrorCode::WrongCategory}),
                      "coverage guard reports only WrongCategory for one isolated category mutation");

    std::vector<detail::ProtocolSurfaceEntry> wrongStability = baseline;
    const auto restabilized =
        findEntry(wrongStability, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "account/login/cancel");
    restabilized->stability = detail::Stability::ExperimentalOnly;
    restabilized->operationContract = {};
    result.expectTrue(hasExactCoverageCodes(wrongStability, manifest, {ErrorCode::WrongStability}),
                      "coverage guard reports only WrongStability for wrong stable/experimental membership");

    std::vector<detail::ProtocolSurfaceEntry> falseTyped = baseline;
    const auto unimplemented =
        findEntry(falseTyped, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "account/login/cancel");
    unimplemented->runtimeDisposition = detail::RuntimeDisposition::Typed;
    unimplemented->typedImplementation = detail::TypedImplementationStatus::Implemented;
    unimplemented->typedSchemaStatus = detail::TypedSchemaStatus::Partial;
    result.expectTrue(hasExactCoverageCodes(falseTyped, manifest, {ErrorCode::TypedWithoutRuntimeTarget}),
                      "coverage guard reports only TypedWithoutRuntimeTarget for a false typed claim");

    std::vector<detail::ProtocolSurfaceEntry> duplicateRuntimeTarget = baseline;
    const auto duplicateTarget =
        findEntry(duplicateRuntimeTarget, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "account/login/cancel");
    duplicateTarget->runtimeDisposition = detail::RuntimeDisposition::Typed;
    duplicateTarget->typedImplementation = detail::TypedImplementationStatus::Implemented;
    duplicateTarget->typedSchemaStatus = detail::TypedSchemaStatus::Partial;
    duplicateTarget->runtimeTarget = detail::ClientRequestTarget::Initialize;
    result.expectTrue(
        hasExactCodes(detail::validateProtocolSurface(duplicateRuntimeTarget), {ErrorCode::DuplicateRuntimeTargetRegistration}),
        "registry validation reports only DuplicateRuntimeTargetRegistration when one declared runtime target occurs twice");

    std::vector<detail::ProtocolSurfaceEntry> wrongRuntimeTargetCategory = baseline;
    const auto wrongTargetCategory =
        findEntry(wrongRuntimeTargetCategory, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "initialize");
    wrongTargetCategory->key.category = detail::SurfaceCategory::TaggedUnionDiscriminator;
    wrongTargetCategory->operationContract = {};
    std::sort(wrongRuntimeTargetCategory.begin(), wrongRuntimeTargetCategory.end(), [](const auto& left, const auto& right) {
        return left.key < right.key;
    });
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(wrongRuntimeTargetCategory), {ErrorCode::WrongRuntimeTargetCategory}),
                      "registry validation reports only WrongRuntimeTargetCategory for a target attached to the wrong category");

    std::vector<detail::ProtocolSurfaceEntry> sentinelRuntimeTarget = baseline;
    const auto sentinelTarget =
        findEntry(sentinelRuntimeTarget, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "account/login/cancel");
    sentinelTarget->runtimeDisposition = detail::RuntimeDisposition::Typed;
    sentinelTarget->typedImplementation = detail::TypedImplementationStatus::Implemented;
    sentinelTarget->typedSchemaStatus = detail::TypedSchemaStatus::Partial;
    sentinelTarget->runtimeTarget = detail::ClientRequestTarget::Count;
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(sentinelRuntimeTarget), {ErrorCode::InvalidRuntimeTarget}),
                      "registry validation reports only InvalidRuntimeTarget for a Count sentinel");

    std::vector<detail::ProtocolSurfaceEntry> typedDispositionMismatch = baseline;
    const auto mistypedDisposition =
        findEntry(typedDispositionMismatch, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "initialize");
    mistypedDisposition->runtimeDisposition = detail::RuntimeDisposition::Deferred;
    result.expectTrue(
        hasExactCodes(detail::validateProtocolSurface(typedDispositionMismatch), {ErrorCode::ImplementedWithoutTypedDisposition}),
        "registry validation reports only ImplementedWithoutTypedDisposition for an implemented target marked deferred");

    std::vector<detail::ProtocolSurfaceEntry> stale = baseline;
    detail::ProtocolSurfaceEntry staleEntry = stale.back();
    staleEntry.key = {detail::SurfaceCategory::TaggedUnionDiscriminator, "zzPhaseA0", "type", "futureStale"};
    staleEntry.stability = detail::Stability::Stable;
    staleEntry.deprecated = false;
    staleEntry.runtimeDisposition = detail::RuntimeDisposition::OpaquePreserved;
    staleEntry.typedImplementation = detail::TypedImplementationStatus::NotImplemented;
    staleEntry.backendCore = detail::LayerStatus::NotApplicable;
    staleEntry.canonicalState = detail::LayerStatus::NotApplicable;
    staleEntry.frontendProtocol = detail::FrontendExposure::NotApplicable;
    staleEntry.frontendSecurity = detail::FrontendSecurityDecision::NotApplicable;
    staleEntry.runtimeTarget = std::monostate{};
    staleEntry.operationContract = {};
    staleEntry.typedSchemaStatus = detail::TypedSchemaStatus::NotImplemented;
    staleEntry.schemaCompleteness = {};
    stale.push_back(staleEntry);
    result.expectTrue(hasExactCoverageCodes(stale, manifest, {ErrorCode::StaleRegistryEntry}),
                      "coverage guard reports only StaleRegistryEntry for an upstream-absent row");

    std::vector<detail::ProtocolSurfaceEntry> demotedTarget = baseline;
    const auto demoted = findEntry(demotedTarget, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "initialize");
    demoted->runtimeDisposition = detail::RuntimeDisposition::Deferred;
    demoted->typedImplementation = detail::TypedImplementationStatus::NotImplemented;
    demoted->typedSchemaStatus = detail::TypedSchemaStatus::NotImplemented;
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(demotedTarget), {ErrorCode::RuntimeTargetWithoutTypedImplementation}),
                      "runtime dispatch target is not represented by an implemented typed registry row: demotion reports only "
                      "RuntimeTargetWithoutTypedImplementation");

    std::vector<detail::ProtocolSurfaceEntry> removedTarget = baseline;
    const auto removed = findEntry(removedTarget, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "initialize");
    removedTarget.erase(removed);
    result.expectTrue(hasExactCodes(detail::validateProtocolSurface(removedTarget), {ErrorCode::MissingRuntimeTargetRegistration}),
                      "runtime dispatch target is not represented by an implemented typed registry row: removal reports only "
                      "MissingRuntimeTargetRegistration at the target-enumeration layer");
    result.expectTrue(
        hasExactCoverageCodes(removedTarget, manifest, {ErrorCode::MissingRegistryEntry, ErrorCode::MissingRuntimeTargetRegistration}),
        "full coverage guard reports exactly missing manifest row plus missing runtime-target registration");

    std::vector<detail::ProtocolSurfaceEntry> ratchetRemoval = baseline;
    const auto removedBaseline = findEntry(ratchetRemoval, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "initialize");
    ratchetRemoval.erase(removedBaseline);
    result.expectTrue(hasExactRatchetCodes(ratchetRemoval, {TypedCoverageRatchetErrorCode::BaselineIdentityMissing}),
                      "typed-coverage ratchet reports only BaselineIdentityMissing after identity removal");

    std::vector<detail::ProtocolSurfaceEntry> ratchetDemotion = baseline;
    const auto demotedBaseline =
        findEntry(ratchetDemotion, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "initialize");
    demotedBaseline->runtimeDisposition = detail::RuntimeDisposition::Deferred;
    demotedBaseline->typedImplementation = detail::TypedImplementationStatus::NotImplemented;
    result.expectTrue(hasExactRatchetCodes(ratchetDemotion, {TypedCoverageRatchetErrorCode::BaselineIdentityNotImplemented}),
                      "typed-coverage ratchet reports only BaselineIdentityNotImplemented after identity demotion");

    std::vector<detail::ProtocolSurfaceEntry> b2AssignmentDrift = baseline;
    const auto reassignedB2 =
        findEntry(b2AssignmentDrift, detail::SurfaceCategory::TaggedUnionDiscriminator, "AskForApproval", "$variant", "granular");
    reassignedB2->typedModule = "ThreadsTurnsSessions";
    result.expectTrue(hasExactRatchetCodes(b2AssignmentDrift, {TypedCoverageRatchetErrorCode::B2IdentityWrongAssignment}),
                      "B2 ratchet rejects an exact identity reassigned outside the A1.1 Common module");

    std::vector<detail::ProtocolSurfaceEntry> b2CompletenessDemotion = baseline;
    const auto incompleteB2 =
        findEntry(b2CompletenessDemotion, detail::SurfaceCategory::TaggedUnionDiscriminator, "AskForApproval", "$variant", "granular");
    incompleteB2->typedSchemaStatus = detail::TypedSchemaStatus::Partial;
    result.expectTrue(hasExactRatchetCodes(b2CompletenessDemotion, {TypedCoverageRatchetErrorCode::B2IdentityNotComplete}),
                      "B2 ratchet rejects a complete identity demoted to Partial");

    std::vector<detail::ProtocolSurfaceEntry> b4AssignmentDrift = baseline;
    const auto reassignedB4 =
        findEntry(b4AssignmentDrift, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "thread/archive");
    reassignedB4->typedModule = "Common";
    result.expectTrue(hasExactRatchetCodes(b4AssignmentDrift, {TypedCoverageRatchetErrorCode::B4IdentityWrongAssignment}),
                      "B4 ratchet rejects an exact operation identity reassigned outside ThreadsTurnsSessions");

    std::vector<detail::ProtocolSurfaceEntry> b4CompletenessDemotion = baseline;
    const auto incompleteB4 =
        findEntry(b4CompletenessDemotion, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "thread/archive");
    incompleteB4->typedSchemaStatus = detail::TypedSchemaStatus::Partial;
    result.expectTrue(hasExactRatchetCodes(b4CompletenessDemotion, {TypedCoverageRatchetErrorCode::B4IdentityNotComplete}),
                      "B4 ratchet rejects a complete operation identity demoted to Partial");

    std::vector<detail::ProtocolSurfaceEntry> b4Removal = baseline;
    b4Removal.erase(
        findEntry(b4Removal, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "thread/archive"));
    result.expectTrue(hasExactRatchetCodes(b4Removal, {TypedCoverageRatchetErrorCode::B4IdentityMissing}),
                      "B4 ratchet rejects removal of an exact operation identity");

    std::vector<detail::ProtocolSurfaceEntry> sameCountReplacement = baseline;
    const std::size_t originalTypedCount = typedIdentityCount(sameCountReplacement);
    const auto replaced = findEntry(sameCountReplacement, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "initialize");
    replaced->key.name = "phaseA0/same-count-replacement";
    result.expectTrue(typedIdentityCount(sameCountReplacement) == originalTypedCount &&
                          hasExactRatchetCodes(sameCountReplacement, {TypedCoverageRatchetErrorCode::BaselineIdentityMissing}),
                      "typed-coverage ratchet rejects same-count replacement with only BaselineIdentityMissing");

    std::vector<detail::ProtocolSurfaceEntry> additiveCoverage = baseline;
    detail::ProtocolSurfaceEntry addedIdentity =
        *findEntry(additiveCoverage, detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", "initialize");
    addedIdentity.key.name = "phaseA0/additive-typed-identity";
    additiveCoverage.push_back(addedIdentity);
    result.expectTrue(typedIdentityCount(additiveCoverage) == typedIdentityCount(baseline) + 1 &&
                          hasExactRatchetCodes(additiveCoverage, {}),
                      "typed-coverage ratchet permits additive coverage without weakening the locked floor");

    return result.processResult();
}
