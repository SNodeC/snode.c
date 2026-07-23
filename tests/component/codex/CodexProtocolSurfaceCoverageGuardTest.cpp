/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "support/TestResult.h"

#include <algorithm>
#include <fstream>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
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

    struct ExpectedEntry {
        detail::Stability stability;
        bool deprecated;
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

    std::vector<std::string> coverageErrors(std::span<const detail::ProtocolSurfaceEntry> registry, const Json& manifest) {
        std::vector<std::string> errors = detail::validateProtocolSurface(registry).errors;
        std::map<OwnedKey, ExpectedEntry> expected;
        for (const Json& manifestEntry : manifest.at("entries")) {
            const detail::Stability stability =
                manifestEntry.at("stability") == "stable" ? detail::Stability::Stable : detail::Stability::ExperimentalOnly;
            const auto [iterator, inserted] =
                expected.emplace(expectedKey(manifestEntry), ExpectedEntry{stability, manifestEntry.at("deprecated").get<bool>()});
            (void) iterator;
            if (!inserted) {
                errors.emplace_back("duplicate schema-derived manifest entry");
            }
        }

        std::map<OwnedKey, const detail::ProtocolSurfaceEntry*> actual;
        for (const detail::ProtocolSurfaceEntry& registryEntry : registry) {
            if (!actual.emplace(ownedKey(registryEntry), &registryEntry).second) {
                errors.emplace_back("duplicate canonical registry identity");
            }
        }

        for (const auto& [key, expectedEntry] : expected) {
            const auto iterator = actual.find(key);
            if (iterator == actual.end()) {
                errors.emplace_back("schema-derived entry absent from canonical registry");
                continue;
            }
            if (iterator->second->stability != expectedEntry.stability) {
                errors.emplace_back("registry stability differs from schema-derived manifest");
            }
            if (iterator->second->deprecated != expectedEntry.deprecated) {
                errors.emplace_back("registry deprecation differs from schema-derived manifest");
            }
        }
        for (const auto& [key, registryEntry] : actual) {
            (void) registryEntry;
            if (!expected.contains(key)) {
                errors.emplace_back("stale registry entry is absent from schema-derived manifest");
            }
        }
        return errors;
    }

    bool guardPasses(const std::vector<detail::ProtocolSurfaceEntry>& entries, const Json& manifest) {
        return coverageErrors(std::span<const detail::ProtocolSurfaceEntry>{entries}, manifest).empty();
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

    result.expectTrue(guardPasses(baseline, manifest), "schema-derived manifest and canonical production runtime registry agree exactly");

    std::vector<detail::ProtocolSurfaceEntry> missing = baseline;
    const auto stable = std::find_if(missing.begin(), missing.end(), [](const detail::ProtocolSurfaceEntry& entry) {
        return entry.stability == detail::Stability::Stable;
    });
    missing.erase(stable);
    result.expectTrue(!guardPasses(missing, manifest), "coverage guard rejects removal of one stable schema-derived entry");

    std::vector<detail::ProtocolSurfaceEntry> duplicate = baseline;
    duplicate.push_back(duplicate.front());
    result.expectTrue(!guardPasses(duplicate, manifest), "coverage guard rejects a duplicate registry entry");

    std::vector<detail::ProtocolSurfaceEntry> wrongCategory = baseline;
    wrongCategory.front().key.category = detail::SurfaceCategory::ClientRequest;
    result.expectTrue(!guardPasses(wrongCategory, manifest), "coverage guard rejects a registry entry in the wrong category");

    std::vector<detail::ProtocolSurfaceEntry> wrongStability = baseline;
    wrongStability.front().stability = detail::Stability::ExperimentalOnly;
    result.expectTrue(!guardPasses(wrongStability, manifest), "coverage guard rejects wrong stable/experimental membership");

    std::vector<detail::ProtocolSurfaceEntry> falseTyped = baseline;
    const auto unimplemented = std::find_if(falseTyped.begin(), falseTyped.end(), [](const detail::ProtocolSurfaceEntry& entry) {
        return std::holds_alternative<std::monostate>(entry.runtimeTarget) &&
               entry.typedImplementation == detail::TypedImplementationStatus::NotImplemented;
    });
    unimplemented->runtimeDisposition = detail::RuntimeDisposition::Typed;
    unimplemented->typedImplementation = detail::TypedImplementationStatus::Implemented;
    result.expectTrue(!guardPasses(falseTyped, manifest), "coverage guard rejects a typed claim without a runtime dispatch target");

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
    stale.push_back(staleEntry);
    result.expectTrue(!guardPasses(stale, manifest), "coverage guard rejects a stale registry entry absent from upstream");

    return result.processResult();
}
