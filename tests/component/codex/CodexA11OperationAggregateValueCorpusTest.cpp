/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/ClientOperationCodec.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/detail/ThreadCodec.h"
#include "ai/openai/codex/detail/TurnCodec.h"
#include "support/TestResult.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
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

    using CwdValue = decltype(typed::ThreadListCwdFilter::value);
    static_assert(!std::is_constructible_v<typed::ThreadListCwdFilter, std::vector<int>>);
    static_assert(!std::is_assignable_v<CwdValue&, std::vector<int>>,
                  "the encode-only CWD filter cannot represent a numeric array element");

    codex::Json readJson(const std::filesystem::path& path) {
        std::ifstream input(path);
        if (!input) {
            throw std::runtime_error("unable to open " + path.string());
        }
        return codex::Json::parse(input);
    }

    std::vector<std::string> stringArray(const codex::Json& value) {
        std::vector<std::string> result;
        result.reserve(value.size());
        for (const codex::Json& element : value) {
            result.emplace_back(element.get<std::string>());
        }
        return result;
    }

    const detail::ClientOperationCodecDescriptor* descriptorFor(const detail::ProtocolSurfaceEntry& row,
                                                                std::span<const detail::ClientOperationCodecDescriptor> descriptors) {
        const auto* target = std::get_if<detail::ClientRequestTarget>(&row.runtimeTarget);
        if (target == nullptr) {
            return nullptr;
        }
        const auto descriptor = std::find_if(descriptors.begin(), descriptors.end(), [&](const auto& candidate) {
            return candidate.target == *target && candidate.key == row.key;
        });
        return descriptor == descriptors.end() ? nullptr : &*descriptor;
    }

    std::vector<std::string> factoryValues(std::string_view domain) {
        if (domain == "ApprovalsReviewer") {
            return {typed::ApprovalsReviewer::user().value,
                    typed::ApprovalsReviewer::autoReview().value,
                    typed::ApprovalsReviewer::guardianSubagent().value};
        }
        if (domain == "Personality") {
            return {typed::Personality::none().value, typed::Personality::friendly().value, typed::Personality::pragmatic().value};
        }
        if (domain == "ReasoningSummary") {
            return {typed::ReasoningSummary::automatic().value,
                    typed::ReasoningSummary::concise().value,
                    typed::ReasoningSummary::detailed().value,
                    typed::ReasoningSummary::none().value};
        }
        if (domain == "SandboxMode") {
            return {typed::SandboxMode::readOnly().value,
                    typed::SandboxMode::workspaceWrite().value,
                    typed::SandboxMode::dangerFullAccess().value};
        }
        if (domain == "SortDirection") {
            return {typed::SortDirection::ascending().value, typed::SortDirection::descending().value};
        }
        if (domain == "ThreadGoalStatus") {
            return {typed::ThreadGoalStatus::active().value,
                    typed::ThreadGoalStatus::paused().value,
                    typed::ThreadGoalStatus::blocked().value,
                    typed::ThreadGoalStatus::usageLimited().value,
                    typed::ThreadGoalStatus::budgetLimited().value,
                    typed::ThreadGoalStatus::complete().value};
        }
        if (domain == "ThreadSortKey") {
            return {
                typed::ThreadSortKey::createdAt().value, typed::ThreadSortKey::updatedAt().value, typed::ThreadSortKey::recencyAt().value};
        }
        if (domain == "ThreadSourceKind") {
            return {typed::ThreadSourceKind::cli().value,
                    typed::ThreadSourceKind::vscode().value,
                    typed::ThreadSourceKind::exec().value,
                    typed::ThreadSourceKind::appServer().value,
                    typed::ThreadSourceKind::subAgent().value,
                    typed::ThreadSourceKind::subAgentReview().value,
                    typed::ThreadSourceKind::subAgentCompact().value,
                    typed::ThreadSourceKind::subAgentThreadSpawn().value,
                    typed::ThreadSourceKind::subAgentOther().value,
                    typed::ThreadSourceKind::unknown().value};
        }
        if (domain == "ThreadStartSource") {
            return {typed::ThreadStartSource::startup().value, typed::ThreadStartSource::clear().value};
        }
        if (domain == "ThreadUnsubscribeStatus") {
            return {typed::ThreadUnsubscribeStatus::notLoaded().value,
                    typed::ThreadUnsubscribeStatus::notSubscribed().value,
                    typed::ThreadUnsubscribeStatus::unsubscribed().value};
        }
        if (domain == "TurnItemsView") {
            return {typed::TurnItemsView::full().value, typed::TurnItemsView::summary().value, typed::TurnItemsView::notLoaded().value};
        }
        if (domain == "TurnStatus") {
            return {typed::TurnStatus::completed().value,
                    typed::TurnStatus::interrupted().value,
                    typed::TurnStatus::failed().value,
                    typed::TurnStatus::inProgress().value};
        }
        return {};
    }

    bool hasFactoryValue(std::string_view domain, std::string_view value) {
        const std::vector<std::string> values = factoryValues(domain);
        return std::find(values.begin(), values.end(), value) != values.end();
    }

    bool
    exactUnknownEnumDiagnostic(const std::vector<typed::DecodeDiagnostic>& diagnostics, std::string_view surface, std::string_view path) {
        return diagnostics.size() == 1 && diagnostics.front().kind == typed::DecodeIssueKind::UnknownEnumValue &&
               diagnostics.front().severity == typed::DecodeIssueSeverity::ForwardCompatibility && diagnostics.front().surface == surface &&
               diagnostics.front().fieldPath == path && diagnostics.front().message.find("synthetic-") == std::string::npos;
    }

    codex::Json validThread() {
        return {{"id", "aggregate-value-thread"},
                {"preview", ""},
                {"ephemeral", false},
                {"modelProvider", "openai"},
                {"createdAt", 0},
                {"updatedAt", 0},
                {"status", {{"type", "idle"}}},
                {"cwd", "/tmp"},
                {"cliVersion", "0.144.6"},
                {"source", "appServer"},
                {"turns", codex::Json::array()},
                {"sessionId", "aggregate-value-session"}};
    }

    struct Observation {
        std::vector<std::string> codes;
        bool exactWireValue = false;
        bool fieldAccounted = false;
        bool incomingRawRetained = true;
        bool known = false;
        bool factoryMatches = false;
        std::optional<std::string> diagnosticPath;
    };

    template <typename OpenValue>
    void recordOpenValueState(Observation& observation, std::string_view domain, const OpenValue& value, bool encoded) {
        observation.known = value.isKnown();
        observation.factoryMatches = hasFactoryValue(domain, value.value) == value.isKnown();
        observation.codes.emplace_back(encoded ? "Encoded" : "Decoded");
    }

    Observation observe(std::string_view domain, const codex::Json& raw) {
        Observation observation;
        const std::string value = raw.is_string() ? raw.get<std::string>() : std::string{};
        std::string error;

        if (domain == "ApprovalsReviewer") {
            typed::ThreadStartParams params;
            params.approvalsReviewer = typed::ApprovalsReviewer{value};
            const auto encoded = detail::encodeThreadStartParams(params, error);
            recordOpenValueState(observation, domain, *params.approvalsReviewer, true);

            codex::Json response = {{"approvalPolicy", "never"},
                                    {"approvalsReviewer", raw},
                                    {"cwd", "/tmp"},
                                    {"instructionSources", codex::Json::array()},
                                    {"model", "gpt-test"},
                                    {"modelProvider", "openai"},
                                    {"sandbox", {{"type", "readOnly"}}},
                                    {"thread", validThread()}};
            const auto operation = detail::decodeClientOperationResult(detail::ClientRequestTarget::ThreadStart, response);
            const auto* decoded = operation.value ? std::get_if<typed::ThreadStartResponse>(&*operation.value) : nullptr;
            observation.codes.emplace_back(std::string(detail::clientOperationDecodeCodeName(operation.diagnostic.code)));
            observation.exactWireValue =
                encoded && encoded->at("approvalsReviewer") == raw && decoded && decoded->approvalsReviewer.value == value;
            observation.fieldAccounted = observation.exactWireValue;
            observation.incomingRawRetained = decoded && decoded->raw == response;
            observation.known = decoded && decoded->approvalsReviewer.isKnown();
            observation.factoryMatches = hasFactoryValue(domain, value) == observation.known;
            if (decoded && !decoded->approvalsReviewer.isKnown() &&
                exactUnknownEnumDiagnostic(decoded->diagnostics, domain, "$.approvalsReviewer")) {
                observation.codes.emplace_back("UnknownEnumValue");
                observation.diagnosticPath = "$.approvalsReviewer";
            }
            return observation;
        }
        if (domain == "Personality") {
            typed::ThreadStartParams params;
            params.personality = typed::Personality{value};
            const auto encoded = detail::encodeThreadStartParams(params, error);
            recordOpenValueState(observation, domain, *params.personality, true);
            observation.exactWireValue = encoded && encoded->at("personality") == raw;
            observation.fieldAccounted = observation.exactWireValue;
            return observation;
        }
        if (domain == "ReasoningSummary") {
            typed::TurnStartParams params;
            params.threadId = typed::ThreadId{"aggregate-value-thread"};
            params.summary = typed::ReasoningSummary{value};
            const auto encoded = detail::encodeTurnStartParams(params, error);
            recordOpenValueState(observation, domain, *params.summary, true);
            observation.exactWireValue = encoded && encoded->at("summary") == raw;
            observation.fieldAccounted = observation.exactWireValue;
            return observation;
        }
        if (domain == "SandboxMode") {
            typed::ThreadStartParams params;
            params.sandbox = typed::SandboxMode{value};
            const auto encoded = detail::encodeThreadStartParams(params, error);
            recordOpenValueState(observation, domain, *params.sandbox, true);
            observation.exactWireValue = encoded && encoded->at("sandbox") == raw;
            observation.fieldAccounted = observation.exactWireValue;
            return observation;
        }
        if (domain == "SortDirection") {
            typed::ThreadListParams params;
            params.sortDirection = typed::SortDirection{value};
            const auto encoded = detail::encodeThreadListParams(params, error);
            recordOpenValueState(observation, domain, *params.sortDirection, true);
            observation.exactWireValue = encoded && encoded->at("sortDirection") == raw;
            observation.fieldAccounted = observation.exactWireValue;
            return observation;
        }
        if (domain == "ThreadGoalStatus") {
            typed::ThreadGoalSetParams params;
            params.threadId = typed::ThreadId{"aggregate-value-thread"};
            params.status = typed::ThreadGoalStatus{value};
            const auto encoded = detail::encodeThreadGoalSetParams(params, error);
            recordOpenValueState(observation, domain, *params.status, true);

            codex::Json goal = {{"createdAt", 0},
                                {"objective", "test"},
                                {"status", raw},
                                {"threadId", "aggregate-value-thread"},
                                {"timeUsedSeconds", 0},
                                {"tokensUsed", 0},
                                {"updatedAt", 0}};
            codex::Json response = {{"goal", goal}};
            const auto operation = detail::decodeClientOperationResult(detail::ClientRequestTarget::ThreadGoalSet, response);
            const auto* decoded = operation.value ? std::get_if<typed::ThreadGoalSetResponse>(&*operation.value) : nullptr;
            observation.codes.emplace_back(std::string(detail::clientOperationDecodeCodeName(operation.diagnostic.code)));
            observation.exactWireValue = encoded && encoded->at("status") == raw && decoded && decoded->goal.status.value == value;
            observation.fieldAccounted = observation.exactWireValue;
            observation.incomingRawRetained = decoded && decoded->raw == response && decoded->goal.raw == goal;
            observation.known = decoded && decoded->goal.status.isKnown();
            observation.factoryMatches = hasFactoryValue(domain, value) == observation.known;
            if (decoded && !decoded->goal.status.isKnown() && exactUnknownEnumDiagnostic(decoded->goal.diagnostics, domain, "$.status")) {
                observation.codes.emplace_back("UnknownEnumValue");
                observation.diagnosticPath = "$.status";
            }
            return observation;
        }
        if (domain == "ThreadSortKey") {
            typed::ThreadListParams params;
            params.sortKey = typed::ThreadSortKey{value};
            const auto encoded = detail::encodeThreadListParams(params, error);
            recordOpenValueState(observation, domain, *params.sortKey, true);
            observation.exactWireValue = encoded && encoded->at("sortKey") == raw;
            observation.fieldAccounted = observation.exactWireValue;
            return observation;
        }
        if (domain == "ThreadSourceKind") {
            typed::ThreadListParams params;
            params.sourceKinds = std::vector<typed::ThreadSourceKind>{{value}};
            const auto encoded = detail::encodeThreadListParams(params, error);
            recordOpenValueState(observation, domain, params.sourceKinds->front(), true);
            observation.exactWireValue = encoded && encoded->at("sourceKinds") == codex::Json::array({raw});
            observation.fieldAccounted = observation.exactWireValue;
            return observation;
        }
        if (domain == "ThreadStartSource") {
            typed::ThreadStartParams params;
            params.sessionStartSource = typed::ThreadStartSource{value};
            const auto encoded = detail::encodeThreadStartParams(params, error);
            recordOpenValueState(observation, domain, *params.sessionStartSource, true);
            observation.exactWireValue = encoded && encoded->at("sessionStartSource") == raw;
            observation.fieldAccounted = observation.exactWireValue;
            return observation;
        }
        if (domain == "ThreadUnsubscribeStatus") {
            const codex::Json response = {{"status", raw}};
            const auto operation = detail::decodeClientOperationResult(detail::ClientRequestTarget::ThreadUnsubscribe, response);
            const auto* decoded = operation.value ? std::get_if<typed::ThreadUnsubscribeResponse>(&*operation.value) : nullptr;
            if (decoded) {
                recordOpenValueState(observation, domain, decoded->status, false);
            }
            observation.exactWireValue = decoded && decoded->status.value == value;
            observation.fieldAccounted = observation.exactWireValue;
            observation.incomingRawRetained = decoded && decoded->raw == response;
            if (decoded && !decoded->status.isKnown() && exactUnknownEnumDiagnostic(decoded->diagnostics, domain, "$.status")) {
                observation.codes.emplace_back("UnknownEnumValue");
                observation.diagnosticPath = "$.status";
            }
            return observation;
        }
        if (domain == "TurnItemsView" || domain == "TurnStatus") {
            codex::Json turn = {{"id", "aggregate-value-turn"},
                                {"status", domain == "TurnStatus" ? raw : codex::Json("completed")},
                                {"items", codex::Json::array()}};
            if (domain == "TurnItemsView") {
                turn["itemsView"] = raw;
            }
            const codex::Json response = {{"turn", turn}};
            const auto operation = detail::decodeClientOperationResult(
                detail::ClientRequestTarget::TurnStart, response, typed::ThreadId{"aggregate-value-thread"});
            const auto* decodedResponse = operation.value ? std::get_if<typed::TurnStartResponse>(&*operation.value) : nullptr;
            const typed::Turn* decoded = decodedResponse == nullptr ? nullptr : &decodedResponse->turn;
            if (domain == "TurnItemsView" && decoded) {
                recordOpenValueState(observation, domain, decoded->itemsView, false);
                observation.exactWireValue = decoded->itemsView.value == value;
            } else if (decoded) {
                recordOpenValueState(observation, domain, decoded->status, false);
                observation.exactWireValue = decoded->status.value == value;
            }
            observation.fieldAccounted = observation.exactWireValue;
            observation.incomingRawRetained = decodedResponse && decodedResponse->raw == response && decoded && decoded->raw == turn;
            if (decoded && !observation.known &&
                exactUnknownEnumDiagnostic(decoded->diagnostics, domain, domain == "TurnItemsView" ? "$.itemsView" : "$.status")) {
                observation.codes.emplace_back("UnknownEnumValue");
                observation.diagnosticPath = domain == "TurnItemsView" ? "$.itemsView" : "$.status";
            }
            return observation;
        }
        if (domain == "ThreadListCwdFilter") {
            typed::ThreadListCwdFilter filter;
            if (raw.is_string()) {
                filter.value = raw.get<std::string>();
            } else if (raw.is_array() && std::all_of(raw.begin(), raw.end(), [](const codex::Json& item) {
                           return item.is_string();
                       })) {
                filter.value = stringArray(raw);
            } else {
                observation.codes = {"MalformedKnownPayload"};
                observation.diagnosticPath = "$.cwd[0]";
                return observation;
            }
            typed::ThreadListParams params;
            params.cwd = std::move(filter);
            const auto encoded = detail::encodeThreadListParams(params, error);
            observation.codes = {"Encoded"};
            observation.exactWireValue = encoded && encoded->at("cwd") == raw;
            observation.fieldAccounted = observation.exactWireValue;
            observation.known = true;
            observation.factoryMatches = true;
            return observation;
        }
        return observation;
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

    const auto descriptors = detail::clientOperationCodecDescriptors();
    std::map<std::string, std::size_t> domains;
    std::size_t knownRecords = 0;
    std::size_t unknownRecords = 0;
    bool exactIndexAgreement = true;
    bool exactRegistryAgreement = true;
    bool exactOutcomeAgreement = true;
    bool exactFieldAgreement = true;
    bool exactFactoryAgreement = true;

    const codex::Json& records = coverage.at("operation_aggregate_value_records");
    for (const codex::Json& record : records) {
        const std::string id = record.at("id").get<std::string>();
        const std::string domain = record.at("domain").get<std::string>();
        const std::string role = record.at("role").get<std::string>();
        ++domains[domain];
        role == "unknown_enum_value" ? ++unknownRecords : ++knownRecords;

        const auto indexed = indexedById.find(id);
        exactIndexAgreement = exactIndexAgreement && indexed != indexedById.end() && indexed->second->at("file") == record.at("file") &&
                              indexed->second->at("file_sha256") == record.at("file_sha256") &&
                              indexed->second->at("role") == record.at("role");

        const codex::Json& key = record.at("owner_surface_key");
        const detail::ProtocolSurfaceEntry* row = detail::findSurface(detail::SurfaceCategory::ClientRequest,
                                                                      key.at("domain").get<std::string>(),
                                                                      key.at("discriminator_field").get<std::string>(),
                                                                      key.at("name").get<std::string>());
        const auto* target = row ? std::get_if<detail::ClientRequestTarget>(&row->runtimeTarget) : nullptr;
        const auto* descriptor = row ? descriptorFor(*row, descriptors) : nullptr;
        exactRegistryAgreement = exactRegistryAgreement && row && target && descriptor &&
                                 detail::clientOperationTargetIdentity(*target) == record.at("runtime_target").get<std::string>() &&
                                 descriptor->target == *target && descriptor->key == row->key;

        const codex::Json raw = readJson(fixtureRoot / record.at("file").get<std::string>());
        Observation observed = observe(domain, raw);
        std::sort(observed.codes.begin(), observed.codes.end());
        std::vector<std::string> expectedCodes = stringArray(record.at("expected_intrinsic_codes"));
        std::sort(expectedCodes.begin(), expectedCodes.end());
        exactOutcomeAgreement = exactOutcomeAgreement && observed.codes == expectedCodes;
        exactFieldAgreement = exactFieldAgreement && observed.exactWireValue && observed.fieldAccounted && observed.incomingRawRetained;

        const bool expectedUnknown = role == "unknown_enum_value";
        exactFactoryAgreement = exactFactoryAgreement && observed.factoryMatches && observed.known != expectedUnknown;
        const codex::Json& expectedPath = record.at("expected_field_path");
        exactFactoryAgreement =
            exactFactoryAgreement &&
            (expectedPath.is_null() ? !observed.diagnosticPath.has_value()
                                    : observed.diagnosticPath && *observed.diagnosticPath == expectedPath.get<std::string>());

        result.expectTrue(exactOutcomeAgreement, id + " has the exact intrinsic production outcome-code multiset");
    }

    result.expectTrue(records.size() == 73 && knownRecords == 49 && unknownRecords == 24,
                      "aggregate/value corpus consumes exactly 73 non-ThreadActiveFlag indexed records");
    result.expectTrue(domains == std::map<std::string, std::size_t>{{"ApprovalsReviewer", 5},
                                                                    {"Personality", 5},
                                                                    {"ReasoningSummary", 6},
                                                                    {"SandboxMode", 5},
                                                                    {"SortDirection", 4},
                                                                    {"ThreadGoalStatus", 8},
                                                                    {"ThreadListCwdFilter", 3},
                                                                    {"ThreadSortKey", 5},
                                                                    {"ThreadSourceKind", 12},
                                                                    {"ThreadStartSource", 4},
                                                                    {"ThreadUnsubscribeStatus", 5},
                                                                    {"TurnItemsView", 5},
                                                                    {"TurnStatus", 6}},
                      "aggregate/value corpus preserves the exact per-family record ratchet");
    result.expectTrue(exactIndexAgreement, "every aggregate/value record agrees with the one independently validated fixture index");
    result.expectTrue(exactRegistryAgreement, "every aggregate/value traversal begins from its exact canonical request row and descriptor");
    result.expectTrue(exactOutcomeAgreement, "all aggregate/value records produce only their exact checked intrinsic outcomes");
    result.expectTrue(exactFieldAgreement,
                      "all aggregate/value fixtures have exact outgoing field accounting and direction-appropriate incoming raw retention");
    result.expectTrue(exactFactoryAgreement,
                      "all known factories, isKnown results, future/empty values, and decoder diagnostics agree exactly");

    return result.processResult();
}
