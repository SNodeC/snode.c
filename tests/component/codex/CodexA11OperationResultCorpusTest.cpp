/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ClientOperationCodec.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "support/TestResult.h"

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef CODEX_A1_FIXTURE_ROOT
#error "CODEX_A1_FIXTURE_ROOT must name the checked-in App Server fixture corpus"
#endif

#ifndef CODEX_A1_OPERATION_PRODUCTION_COVERAGE
#error "CODEX_A1_OPERATION_PRODUCTION_COVERAGE must name the checked production-coverage table"
#endif

namespace {
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    constexpr std::string_view DecodedCode = "Decoded";
    constexpr std::string_view MalformedKnownPayloadCode = "MalformedKnownPayload";

    codex::Json readJson(const std::filesystem::path& path) {
        std::ifstream input(path);
        if (!input) {
            throw std::runtime_error("unable to open " + path.string());
        }
        return codex::Json::parse(input);
    }

    struct DecodeObservation {
        std::vector<std::string> codes;
        bool rawRetained = false;
        detail::ClientOperationDecodeDiagnostic diagnostic;
    };

    DecodeObservation decodeThroughProductionTarget(detail::ClientRequestTarget target, const codex::Json& raw) {
        detail::ClientOperationDecodeResult decoded =
            detail::decodeClientOperationResult(target, raw, typed::ThreadId{"corpus-parent-thread"});
        bool rawRetained = false;
        if (decoded.value) {
            rawRetained = std::visit(
                [&](const auto& value) {
                    using Value = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<Value, typed::Unit>) {
                        return raw == codex::Json::object();
                    } else if constexpr (std::is_same_v<Value, typed::LoginAccountResponse>) {
                        return typed::loginAccountResponseRaw(value) == raw;
                    } else {
                        return value.raw == raw;
                    }
                },
                *decoded.value);
        }
        return {{std::string(detail::clientOperationDecodeCodeName(decoded.diagnostic.code))}, rawRetained, std::move(decoded.diagnostic)};
    }

    const detail::ClientOperationCodecDescriptor* descriptorFor(const detail::ProtocolSurfaceEntry& row,
                                                                const std::span<const detail::ClientOperationCodecDescriptor> descriptors) {
        const auto* target = std::get_if<detail::ClientRequestTarget>(&row.runtimeTarget);
        if (target == nullptr) {
            return nullptr;
        }
        const auto descriptor = std::find_if(descriptors.begin(), descriptors.end(), [&](const auto& candidate) {
            return candidate.target == *target;
        });
        return descriptor == descriptors.end() || descriptor->key != row.key ? nullptr : &*descriptor;
    }

    std::vector<std::string> stringArray(const codex::Json& value) {
        std::vector<std::string> result;
        for (const codex::Json& element : value) {
            result.emplace_back(element.get<std::string>());
        }
        return result;
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

    const std::span<const detail::ClientOperationCodecDescriptor> descriptors = detail::clientOperationCodecDescriptors();
    const std::size_t a11Descriptors =
        static_cast<std::size_t>(std::count_if(descriptors.begin(), descriptors.end(), [](const auto& descriptor) {
            return detail::entryFor(descriptor.target).a1Slice == detail::A1Slice::A1_1;
        }));
    result.expectTrue(a11Descriptors == 22,
                      "the production result corpus begins from exactly 22 generated A1.1 operation descriptors, independent of later slices");

    std::map<std::string, std::size_t> roles;
    std::set<detail::ClientRequestTarget> baseTargets;
    std::size_t unitRoots = 0;
    std::size_t concreteRoots = 0;
    std::size_t positiveRecords = 0;
    std::size_t negativeRecords = 0;
    bool exactIndexAgreement = true;
    bool exactRegistryAgreement = true;
    bool exactOutcomeAgreement = true;
    bool rawRetentionAgreement = true;

    for (const codex::Json& record : coverage.at("records")) {
        const std::string id = record.at("id").get<std::string>();
        const std::string method = record.at("surface_key").at("name").get<std::string>();
        const std::string role = record.at("role").get<std::string>();
        ++roles[role];

        const auto indexed = indexedById.find(id);
        exactIndexAgreement = exactIndexAgreement && indexed != indexedById.end() && indexed->second->at("file") == record.at("file") &&
                              indexed->second->at("file_sha256") == record.at("file_sha256") &&
                              indexed->second->at("role") == record.at("role") &&
                              indexed->second->at("protocol_surface_key") == record.at("surface_key");

        const detail::ProtocolSurfaceEntry* row =
            detail::findSurface(detail::SurfaceCategory::ClientRequest, "ClientRequest", "method", method);
        const auto* target = row == nullptr ? nullptr : std::get_if<detail::ClientRequestTarget>(&row->runtimeTarget);
        const detail::ClientOperationCodecDescriptor* descriptor = row == nullptr ? nullptr : descriptorFor(*row, descriptors);
        exactRegistryAgreement = exactRegistryAgreement && row && target && descriptor && row->stability == detail::Stability::Stable &&
                                 row->a1Slice == detail::A1Slice::A1_1 && row->runtimeDisposition == detail::RuntimeDisposition::Typed &&
                                 row->typedImplementation == detail::TypedImplementationStatus::Implemented &&
                                 record.at("runtime_target") == detail::clientOperationTargetIdentity(*target) &&
                                 record.at("result_type_identity") == descriptor->resultTypeIdentity;
        if (row == nullptr || target == nullptr || descriptor == nullptr || indexed == indexedById.end()) {
            continue;
        }

        if (role == "client_request_result") {
            baseTargets.emplace(*target);
            if (descriptor->resultKind == detail::ResultContractKind::Unit) {
                ++unitRoots;
            } else if (descriptor->resultKind == detail::ResultContractKind::Concrete) {
                ++concreteRoots;
            }
        }

        const bool positive = role == "client_request_result" || role == "operation_optional_omitted" || role == "operation_nullable_null";
        positive ? ++positiveRecords : ++negativeRecords;

        const codex::Json raw = readJson(fixtureRoot / record.at("file").get<std::string>());
        const DecodeObservation observation = decodeThroughProductionTarget(*target, raw);
        const std::vector<std::string> expectedCodes = stringArray(record.at("expected_intrinsic_codes"));
        exactOutcomeAgreement = exactOutcomeAgreement && observation.codes == expectedCodes && observation.diagnostic.target == *target &&
                                observation.diagnostic.surfaceKey == row->key && observation.diagnostic.fieldPath == "$" &&
                                (positive ? expectedCodes == std::vector<std::string>{std::string(DecodedCode)}
                                          : expectedCodes == std::vector<std::string>{std::string(MalformedKnownPayloadCode)});
        rawRetentionAgreement = rawRetentionAgreement && (!positive || observation.rawRetained);
        result.expectTrue(observation.codes == expectedCodes, id + " production decoder emits the exact intrinsic code multiset");
        result.expectTrue(!positive || observation.rawRetained, id + " accepted result retains its exact raw fixture");
    }

    result.expectTrue(exactIndexAgreement, "every production-coverage row is the exact indexed fixture id/path/hash/role/surface record");
    result.expectTrue(exactRegistryAgreement,
                      "every corpus record resolves through one exact stable A1.1 registry row and generated production target");
    result.expectTrue(coverage.at("records").size() == 1166 && positiveRecords == 403 && negativeRecords == 763 &&
                          roles ==
                              std::map<std::string, std::size_t>{
                                  {"client_request_result", 22},
                                  {"operation_missing_required", 261},
                                  {"operation_nullable_null", 180},
                                  {"operation_optional_omitted", 201},
                                  {"operation_wrong_type", 502},
                              },
                      "the exact result corpus role ratchet remains 22 base, 201 omitted, 180 null, 261 missing, and 502 wrong-type");
    result.expectTrue(baseTargets.size() == 22 && unitRoots == 7 && concreteRoots == 15,
                      "all 22 production targets are reached exactly as seven Unit and fifteen Concrete result roots");
    result.expectTrue(
        exactOutcomeAgreement,
        "all 403 schema-valid results emit exactly {Decoded} and all 763 known mutations emit exactly {MalformedKnownPayload}");
    result.expectTrue(rawRetentionAgreement,
                      "every accepted Concrete result retains the exact corpus JSON and each Unit accepts only the exact empty object");

    const auto secretDiagnostic = detail::decodeClientOperationResult(detail::ClientRequestTarget::ThreadUnsubscribe,
                                                                      {{"status", {{"secret", "sensitive-operation-secret"}}}});
    result.expectTrue(secretDiagnostic.diagnostic.code == detail::ClientOperationDecodeCode::MalformedKnownPayload &&
                          secretDiagnostic.diagnostic.surfaceKey == detail::entryFor(detail::ClientRequestTarget::ThreadUnsubscribe).key &&
                          secretDiagnostic.diagnostic.message.find("sensitive-operation-secret") == std::string::npos,
                      "structured operation diagnostics retain identity and decoder context without payload values");

    return result.processResult();
}
