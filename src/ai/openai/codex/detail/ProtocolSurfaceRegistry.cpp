/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace ai::openai::codex::detail {

    namespace {

#define CODEX_PROTOCOL_SURFACE_ENTRY(category,                                                                                             \
                                     domain,                                                                                               \
                                     field,                                                                                                \
                                     name,                                                                                                 \
                                     stability,                                                                                            \
                                     deprecated,                                                                                           \
                                     runtimeDisposition,                                                                                   \
                                     typedImplementation,                                                                                  \
                                     backendCore,                                                                                          \
                                     canonicalState,                                                                                       \
                                     frontendProtocol,                                                                                     \
                                     frontendSecurity,                                                                                     \
                                     runtimeTarget)                                                                                        \
    {{category, domain, field, name},                                                                                                      \
     stability,                                                                                                                            \
     deprecated,                                                                                                                           \
     runtimeDisposition,                                                                                                                   \
     typedImplementation,                                                                                                                  \
     backendCore,                                                                                                                          \
     canonicalState,                                                                                                                       \
     frontendProtocol,                                                                                                                     \
     frontendSecurity,                                                                                                                     \
     RuntimeTarget{runtimeTarget}},

        constexpr ProtocolSurfaceEntry Registry[] = {
#include "ai/openai/codex/detail/ProtocolSurfaceRegistryData.inc"
        };

#undef CODEX_PROTOCOL_SURFACE_ENTRY

        const char* categoryName(SurfaceCategory category) noexcept {
            switch (category) {
                case SurfaceCategory::ClientRequest:
                    return "client request";
                case SurfaceCategory::ClientNotification:
                    return "client notification";
                case SurfaceCategory::ServerNotification:
                    return "server notification";
                case SurfaceCategory::ServerRequest:
                    return "server request";
                case SurfaceCategory::ItemDiscriminator:
                    return "item discriminator";
                case SurfaceCategory::DeltaProgressDiscriminator:
                    return "delta/progress discriminator";
                case SurfaceCategory::TaggedUnionDiscriminator:
                    return "tagged-union discriminator";
            }

            return "unknown";
        }

        bool isMethodCategory(SurfaceCategory category) noexcept {
            return category == SurfaceCategory::ClientRequest || category == SurfaceCategory::ClientNotification ||
                   category == SurfaceCategory::ServerNotification || category == SurfaceCategory::ServerRequest;
        }

        std::string keyName(const ProtocolSurfaceKey& key) {
            std::string result(categoryName(key.category));
            result += " ";
            if (!key.domain.empty()) {
                result += std::string(key.domain);
                result += ".";
            }
            if (!key.field.empty()) {
                result += std::string(key.field);
                result += "=";
            }
            result += std::string(key.name);
            return result;
        }

        std::optional<SurfaceCategory> targetCategory(const RuntimeTarget& target) noexcept {
            return std::visit(
                []<typename Target>(const Target&) -> std::optional<SurfaceCategory> {
                    if constexpr (std::is_same_v<Target, std::monostate>) {
                        return std::nullopt;
                    } else if constexpr (std::is_same_v<Target, ClientRequestTarget>) {
                        return SurfaceCategory::ClientRequest;
                    } else if constexpr (std::is_same_v<Target, ClientNotificationTarget>) {
                        return SurfaceCategory::ClientNotification;
                    } else if constexpr (std::is_same_v<Target, ServerNotificationTarget>) {
                        return SurfaceCategory::ServerNotification;
                    } else if constexpr (std::is_same_v<Target, ServerRequestTarget>) {
                        return SurfaceCategory::ServerRequest;
                    } else if constexpr (std::is_same_v<Target, ItemDiscriminatorTarget>) {
                        return SurfaceCategory::ItemDiscriminator;
                    } else {
                        static_assert(std::is_same_v<Target, void>, "new runtime target type needs an explicit surface category");
                    }
                },
                target);
        }

        bool validTarget(const RuntimeTarget& target) noexcept {
            return std::visit(
                []<typename Target>(const Target& value) {
                    if constexpr (std::is_same_v<Target, std::monostate>) {
                        return true;
                    } else {
                        return value != Target::Count;
                    }
                },
                target);
        }

        template <typename Target>
        const ProtocolSurfaceEntry& findTarget(Target target) {
            const std::span<const ProtocolSurfaceEntry> entries = protocolSurfaceRegistry();
            const auto iterator = std::find_if(entries.begin(), entries.end(), [target](const ProtocolSurfaceEntry& entry) {
                const Target* entryTarget = std::get_if<Target>(&entry.runtimeTarget);
                return entryTarget != nullptr && *entryTarget == target;
            });
            if (iterator == entries.end()) {
                throw std::logic_error("Codex protocol runtime target is absent from the canonical registry");
            }
            if (iterator->runtimeDisposition != RuntimeDisposition::Typed ||
                iterator->typedImplementation != TypedImplementationStatus::Implemented) {
                throw std::logic_error("Codex protocol runtime target is not marked as a typed implementation");
            }
            return *iterator;
        }

        template <typename Target>
        void validateTargets(std::span<const ProtocolSurfaceEntry> entries,
                             Target countTarget,
                             std::string_view category,
                             ProtocolSurfaceValidation& result) {
            const std::size_t targetCount = static_cast<std::size_t>(countTarget);
            for (std::size_t targetIndex = 0; targetIndex < targetCount; ++targetIndex) {
                const Target target = static_cast<Target>(targetIndex);
                const std::size_t count =
                    static_cast<std::size_t>(std::count_if(entries.begin(), entries.end(), [target](const ProtocolSurfaceEntry& entry) {
                        const Target* entryTarget = std::get_if<Target>(&entry.runtimeTarget);
                        return entryTarget != nullptr && *entryTarget == target;
                    }));
                if (count != 1) {
                    result.errors.push_back({count == 0 ? ProtocolSurfaceErrorCode::MissingRuntimeTargetRegistration
                                                        : ProtocolSurfaceErrorCode::DuplicateRuntimeTargetRegistration,
                                             std::string(category) + " runtime target " + std::to_string(static_cast<std::size_t>(target)) +
                                                 (count == 0 ? " is absent from the protocol surface registry"
                                                             : " is duplicated in the protocol surface registry")});
                }
            }
        }

    } // namespace

    std::span<const ProtocolSurfaceEntry> protocolSurfaceRegistry() noexcept {
        return Registry;
    }

    const ProtocolSurfaceEntry*
    findSurface(SurfaceCategory category, std::string_view domain, std::string_view field, std::string_view name) noexcept {
        const std::span<const ProtocolSurfaceEntry> entries = protocolSurfaceRegistry();
        const ProtocolSurfaceKey key{category, domain, field, name};
        const auto iterator = std::lower_bound(
            entries.begin(), entries.end(), key, [](const ProtocolSurfaceEntry& entry, const ProtocolSurfaceKey& expected) {
                return entry.key < expected;
            });
        return iterator == entries.end() || iterator->key != key ? nullptr : &*iterator;
    }

    const ProtocolSurfaceEntry& entryFor(ClientRequestTarget target) {
        return findTarget(target);
    }

    const ProtocolSurfaceEntry& entryFor(ClientNotificationTarget target) {
        return findTarget(target);
    }

    const ProtocolSurfaceEntry& entryFor(ServerNotificationTarget target) {
        return findTarget(target);
    }

    const ProtocolSurfaceEntry& entryFor(ServerRequestTarget target) {
        return findTarget(target);
    }

    const ProtocolSurfaceEntry& entryFor(ItemDiscriminatorTarget target) {
        return findTarget(target);
    }

    ProtocolSurfaceValidation validateProtocolSurface(std::span<const ProtocolSurfaceEntry> entries) {
        static_assert(std::variant_size_v<RuntimeTarget> == 6, "new runtime target type needs an explicit validateTargets invocation");

        ProtocolSurfaceValidation result;

        for (std::size_t index = 0; index < entries.size(); ++index) {
            const ProtocolSurfaceEntry& entry = entries[index];
            if (entry.key.name.empty()) {
                result.errors.push_back({ProtocolSurfaceErrorCode::EmptyName, "protocol surface entry has an empty name"});
            }
            if (entry.key.field.empty()) {
                result.errors.push_back(
                    {ProtocolSurfaceErrorCode::EmptyDiscriminatorField, keyName(entry.key) + " has an empty discriminator field"});
            }

            const std::optional<SurfaceCategory> expectedCategory = targetCategory(entry.runtimeTarget);
            if (!validTarget(entry.runtimeTarget)) {
                result.errors.push_back(
                    {ProtocolSurfaceErrorCode::InvalidRuntimeTarget, keyName(entry.key) + " uses an invalid runtime target sentinel"});
            }
            if (expectedCategory && *expectedCategory != entry.key.category) {
                result.errors.push_back({ProtocolSurfaceErrorCode::WrongRuntimeTargetCategory,
                                         keyName(entry.key) + " has a runtime target for the wrong category"});
            }
            if (entry.typedImplementation == TypedImplementationStatus::Implemented && !expectedCategory) {
                result.errors.push_back({ProtocolSurfaceErrorCode::TypedWithoutRuntimeTarget,
                                         keyName(entry.key) + " claims typed implementation without a runtime target"});
            }
            if (expectedCategory && entry.typedImplementation != TypedImplementationStatus::Implemented) {
                result.errors.push_back(
                    {ProtocolSurfaceErrorCode::RuntimeTargetWithoutTypedImplementation,
                     keyName(entry.key) + " runtime dispatch target is not represented by an implemented typed registry row"});
            }
            if (entry.runtimeDisposition == RuntimeDisposition::Typed &&
                entry.typedImplementation != TypedImplementationStatus::Implemented) {
                result.errors.push_back({ProtocolSurfaceErrorCode::TypedDispositionWithoutImplementation,
                                         keyName(entry.key) + " has typed runtime disposition without typed implementation"});
            }
            if (entry.typedImplementation == TypedImplementationStatus::Implemented &&
                entry.runtimeDisposition != RuntimeDisposition::Typed) {
                result.errors.push_back({ProtocolSurfaceErrorCode::ImplementedWithoutTypedDisposition,
                                         keyName(entry.key) + " claims typed implementation without typed runtime disposition"});
            }
            switch (entry.frontendProtocol) {
                case FrontendExposure::ExistingOperationSubset:
                    if (entry.frontendSecurity != FrontendSecurityDecision::ExistingOperationSubsetExpansionUnresolved) {
                        result.errors.push_back(
                            {ProtocolSurfaceErrorCode::FrontendSecurityMismatch,
                             keyName(entry.key) + " has existing operation-subset frontend exposure without its expansion decision"});
                    }
                    break;
                case FrontendExposure::GenericUnknownRequest:
                    if (entry.frontendSecurity != FrontendSecurityDecision::ExistingGenericContractDedicatedUnresolved) {
                        result.errors.push_back(
                            {ProtocolSurfaceErrorCode::FrontendSecurityMismatch,
                             keyName(entry.key) + " has generic-unknown-request frontend exposure without its dedicated-method decision"});
                    }
                    break;
                case FrontendExposure::ExistingEventSubset:
                    if (entry.frontendSecurity != FrontendSecurityDecision::ExistingEventSubsetContract) {
                        result.errors.push_back(
                            {ProtocolSurfaceErrorCode::FrontendSecurityMismatch,
                             keyName(entry.key) + " has existing event-subset frontend exposure without its existing contract decision"});
                    }
                    break;
                case FrontendExposure::GenericExtension:
                    if (entry.frontendSecurity != FrontendSecurityDecision::ExistingRedactedExtensionContract) {
                        result.errors.push_back(
                            {ProtocolSurfaceErrorCode::FrontendSecurityMismatch,
                             keyName(entry.key) +
                                 " has generic-extension frontend exposure without its existing redacted-extension contract decision"});
                    }
                    break;
                case FrontendExposure::ExistingUnknownItemSubset:
                    if (entry.frontendSecurity != FrontendSecurityDecision::ExistingUnknownItemMetadataContract) {
                        result.errors.push_back(
                            {ProtocolSurfaceErrorCode::FrontendSecurityMismatch,
                             keyName(entry.key) +
                                 " has unknown-item subset frontend exposure without its existing metadata-only contract decision"});
                    }
                    break;
                case FrontendExposure::NotExposed:
                    if (entry.frontendSecurity != FrontendSecurityDecision::Unresolved) {
                        result.errors.push_back({ProtocolSurfaceErrorCode::FrontendSecurityMismatch,
                                                 keyName(entry.key) + " is not frontend-exposed but lacks an unresolved owner decision"});
                    }
                    break;
                case FrontendExposure::NotApplicable:
                    if (entry.frontendSecurity != FrontendSecurityDecision::NotApplicable) {
                        result.errors.push_back(
                            {ProtocolSurfaceErrorCode::FrontendSecurityMismatch,
                             keyName(entry.key) + " has inapplicable frontend exposure with an applicable security decision"});
                    }
                    break;
            }
            if (index != 0 && entry.key < entries[index - 1].key) {
                result.errors.push_back(
                    {ProtocolSurfaceErrorCode::UnsortedRegistry, "protocol surface registry is not sorted at " + keyName(entry.key)});
            }

            for (std::size_t previous = 0; previous < index; ++previous) {
                const ProtocolSurfaceEntry& candidate = entries[previous];
                if (candidate.key == entry.key) {
                    result.errors.push_back(
                        {ProtocolSurfaceErrorCode::DuplicateRegistryEntry, "duplicate protocol surface entry: " + keyName(entry.key)});
                }
                if (candidate.key.category != entry.key.category && candidate.key.domain == entry.key.domain &&
                    candidate.key.field == entry.key.field && candidate.key.name == entry.key.name) {
                    result.errors.push_back({ProtocolSurfaceErrorCode::DiscriminatorCategoryCollision,
                                             "protocol discriminator category collision: " + keyName(entry.key)});
                }
                if (candidate.key.category != entry.key.category && isMethodCategory(candidate.key.category) &&
                    isMethodCategory(entry.key.category) && candidate.key.name == entry.key.name) {
                    result.errors.push_back(
                        {ProtocolSurfaceErrorCode::MethodCategoryCollision, "protocol method category collision: " + keyName(entry.key)});
                }
            }
        }

        validateTargets(entries, ClientRequestTarget::Count, "client request", result);
        validateTargets(entries, ClientNotificationTarget::Count, "client notification", result);
        validateTargets(entries, ServerNotificationTarget::Count, "server notification", result);
        validateTargets(entries, ServerRequestTarget::Count, "server request", result);
        validateTargets(entries, ItemDiscriminatorTarget::Count, "item discriminator", result);

        return result;
    }

} // namespace ai::openai::codex::detail
