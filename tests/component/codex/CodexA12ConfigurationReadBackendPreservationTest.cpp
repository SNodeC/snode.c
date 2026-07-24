/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/backend/Reducer.h"
#include "ai/openai/codex/backend/Snapshot.h"
#include "ai/openai/codex/detail/EventDecoder.h"
#include "ai/openai/codex/typed/Configuration.h"
#include "support/TestResult.h"

#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    constexpr const char* SyntheticWarningDetails = "synthetic-config-warning-details-sensitive";

    backend::Snapshot withoutExtensions(backend::Snapshot snapshot) {
        snapshot.recentExtensions.clear();
        snapshot.omittedRecentExtensions = 0;
        return snapshot;
    }

    codex::Notification configWarning(codex::Json params, std::size_t sequence) {
        codex::Json raw{
            {"jsonrpc", "2.0"},
            {"method", "configWarning"},
            {"params", params},
            {"futureEnvelopeOnly", {{"sequence", sequence}, {"mustNotBecomeParams", true}}},
        };
        return {"configWarning", std::move(params), std::move(raw)};
    }

    codex::Json frontendCompatibleData(const backend::ExtensionSnapshot& extension) {
        codex::Json data{{"method", extension.method}, {"params", extension.payload}};
        if (extension.sensitiveFieldsRedacted) {
            data["sensitiveFieldsRedacted"] = true;
        }
        return data;
    }

    void testTypedParamsOnlyPreservationAndProjection(tests::support::TestResult& result) {
        const codex::Json params{
            {"details", SyntheticWarningDetails},
            {"path", "/tmp/test-config.toml"},
            {"range", {{"start", {{"line", 3}, {"column", 5}}}, {"end", {{"line", 3}, {"column", 17}}}}},
            {"summary", "Synthetic configuration warning"},
        };
        const codex::Notification wire = configWarning(params, 1);
        const typed::Event event = detail::decodeEvent(wire);
        const auto* decoded = std::get_if<typed::ConfigWarningNotification>(&event);

        const bool exactTypedValue =
            decoded && decoded->raw == wire.raw && decoded->details.present && decoded->details.value.has_value() &&
            *decoded->details.value == SyntheticWarningDetails && decoded->path.present && decoded->path.value.has_value() &&
            decoded->path.value->value == "/tmp/test-config.toml" && decoded->range.present && decoded->range.value.has_value() &&
            decoded->range.value->start.line == 3 && decoded->range.value->start.column == 5 && decoded->range.value->end.line == 3 &&
            decoded->range.value->end.column == 17 && decoded->summary == "Synthetic configuration warning";
        result.expectTrue(exactTypedValue, "typed configWarning retains every known params field and the complete JSON-RPC envelope");

        backend::Reducer reducer;
        backend::BackendState state;
        const backend::Snapshot before = backend::makeSnapshot(state);
        const std::vector<backend::BackendEvent> translated = reducer.translate(event);
        const auto* extension = translated.size() == 1 ? std::get_if<backend::CodexExtensionReceived>(&translated.front()) : nullptr;

        result.expectTrue(
            extension && extension->method == "configWarning" && extension->payload == wire.params &&
                extension->payload.value("details", "") == SyntheticWarningDetails && !extension->payload.contains("jsonrpc") &&
                !extension->payload.contains("method") && !extension->payload.contains("futureEnvelopeOnly") &&
                extension->payload.dump().find("mustNotBecomeParams") == std::string::npos,
            "configWarning translation preserves exact internal params, including details, without nesting envelope-only fields");

        if (extension) {
            const backend::Reduction reduction = reducer.apply(state, *extension);
            result.expectTrue(reduction.changed && !reduction.flushImmediately,
                              "configWarning changes only the existing bounded extension history");
        }

        const backend::ExtensionRecord* retained = state.recentExtensions.size() == 1 ? &state.recentExtensions.front() : nullptr;
        result.expectTrue(retained && retained->method == "configWarning" && retained->payload == wire.params &&
                              retained->payload.value("details", "") == SyntheticWarningDetails &&
                              !retained->payload.contains("futureEnvelopeOnly"),
                          "BackendCore retains the exact typed configWarning params before frontend projection");

        const backend::Snapshot snapshot = backend::makeSnapshot(state);
        const backend::ExtensionSnapshot* projected = snapshot.recentExtensions.size() == 1 ? &snapshot.recentExtensions.front() : nullptr;
        codex::Json frontendData = codex::Json::object();
        if (projected) {
            frontendData = frontendCompatibleData(*projected);
        }
        const std::string serializedFrontendData = frontendData.dump();

        const bool exactFrontendProjection =
            projected && projected->method == "configWarning" && projected->sensitiveFieldsRedacted &&
            projected->payload.value("details", "") == "[redacted]" &&
            projected->payload.value("summary", "") == "Synthetic configuration warning" &&
            projected->payload.value("path", "") == "/tmp/test-config.toml" && projected->payload.at("range") == params.at("range") &&
            !projected->payload.contains("jsonrpc") && !projected->payload.contains("method") &&
            !projected->payload.contains("futureEnvelopeOnly") && frontendData.value("params", codex::Json{}) == projected->payload &&
            frontendData.value("sensitiveFieldsRedacted", false) &&
            serializedFrontendData.find(SyntheticWarningDetails) == std::string::npos &&
            serializedFrontendData.find("mustNotBecomeParams") == std::string::npos;
        result.expectTrue(exactFrontendProjection,
                          "makeSnapshot method-specifically redacts configWarning details while retaining safe params semantics in "
                          "serialized frontend-compatible data");

        result.expectTrue(withoutExtensions(snapshot) == before && state.threads.empty() && state.threadOrder.empty() &&
                              state.pendingRequests.empty(),
                          "configWarning preservation invents no canonical configuration, warning, thread, or pending-request state");
    }

    void testExistingExtensionBound(tests::support::TestResult& result) {
        backend::Reducer reducer;
        backend::BackendState state;
        const backend::Snapshot before = backend::makeSnapshot(state);
        std::size_t translatedCount = 0;

        for (std::size_t index = 0; index < 65; ++index) {
            const codex::Notification wire =
                configWarning({{"details", nullptr}, {"summary", "warning-" + std::to_string(index)}, {"sequence", index}}, index);
            const std::vector<backend::BackendEvent> translated = reducer.translate(detail::decodeEvent(wire));
            if (translated.size() == 1 && std::holds_alternative<backend::CodexExtensionReceived>(translated.front())) {
                ++translatedCount;
                reducer.apply(state, translated.front());
            }
        }

        const bool exactBound = translatedCount == 65 && state.recentExtensions.size() == 64 &&
                                state.recentExtensions.front().method == "configWarning" &&
                                state.recentExtensions.front().payload.value("sequence", 0U) == 1U &&
                                state.recentExtensions.front().payload.value("summary", "") == "warning-1" &&
                                state.recentExtensions.back().payload.value("sequence", 0U) == 64U &&
                                state.recentExtensions.back().payload.value("summary", "") == "warning-64";
        result.expectTrue(exactBound, "configWarning uses the existing exact 64-record bounded extension history");
        result.expectTrue(withoutExtensions(backend::makeSnapshot(state)) == before && state.threads.empty() &&
                              state.pendingRequests.empty(),
                          "bounded configWarning preservation still creates no canonical configuration or warning state");
    }

    void testMethodSpecificTriStateRedaction(tests::support::TestResult& result) {
        const backend::ExtensionSnapshot explicitNull = backend::makeExtensionSnapshot(backend::ExtensionRecord{
            .method = "configWarning",
            .payload = {{"details", nullptr}, {"summary", "null details"}},
        });
        const backend::ExtensionSnapshot omitted = backend::makeExtensionSnapshot(backend::ExtensionRecord{
            .method = "configWarning",
            .payload = {{"summary", "omitted details"}},
        });
        const backend::ExtensionSnapshot unrelated = backend::makeExtensionSnapshot(backend::ExtensionRecord{
            .method = "future/extension",
            .payload = {{"details", "ordinary unrelated details"}, {"summary", "safe"}},
        });

        result.expectTrue(!explicitNull.sensitiveFieldsRedacted && explicitNull.payload.contains("details") &&
                              explicitNull.payload.at("details").is_null() && explicitNull.payload.value("summary", "") == "null details",
                          "explicit-null configWarning details retain null without a spurious redaction marker");
        result.expectTrue(!omitted.sensitiveFieldsRedacted && !omitted.payload.contains("details") &&
                              omitted.payload.value("summary", "") == "omitted details",
                          "omitted configWarning details remain omitted without a spurious redaction marker");
        result.expectTrue(!unrelated.sensitiveFieldsRedacted && unrelated.payload.value("details", "") == "ordinary unrelated details" &&
                              unrelated.payload.value("summary", "") == "safe",
                          "the configWarning details policy does not alter an unrelated extension's details field");
    }
} // namespace

int main() {
    static_assert(std::is_constructible_v<typed::Event, typed::ConfigWarningNotification>);

    tests::support::TestResult result;
    testTypedParamsOnlyPreservationAndProjection(result);
    testExistingExtensionBound(result);
    testMethodSpecificTriStateRedaction(result);
    return result.processResult();
}
