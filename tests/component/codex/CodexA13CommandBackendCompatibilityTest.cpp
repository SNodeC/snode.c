/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/backend/Reducer.h"
#include "ai/openai/codex/backend/Snapshot.h"
#include "ai/openai/codex/detail/EventDecoder.h"
#include "ai/openai/codex/typed/Commands.h"
#include "ai/openai/codex/typed/Events.h"
#include "support/TestResult.h"

#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    constexpr const char* SyntheticDelta = "c3ludGhldGljLXN0cmVhbS1ieXRlcw==";
    constexpr const char* SyntheticProcessId = "synthetic-command-process";

    backend::Snapshot withoutExtensions(backend::Snapshot snapshot) {
        snapshot.recentExtensions.clear();
        snapshot.omittedRecentExtensions = 0;
        return snapshot;
    }

    codex::Json frontendCompatibleData(const backend::ExtensionSnapshot& extension) {
        codex::Json data{{"method", extension.method}, {"params", extension.payload}};
        if (extension.sensitiveFieldsRedacted) {
            data["sensitiveFieldsRedacted"] = true;
        }
        return data;
    }

    void testCommandOutputUsesExistingRedactedExtensionBoundary(tests::support::TestResult& result) {
        const codex::Json params{
            {"capReached", true},
            {"deltaBase64", SyntheticDelta},
            {"processId", SyntheticProcessId},
            {"stream", "stderr"},
            {"futureSafeField", "visible"},
        };
        const codex::Json raw{
            {"jsonrpc", "2.0"},
            {"method", "command/exec/outputDelta"},
            {"params", params},
            {"futureEnvelopeOnly", "must-not-become-params"},
        };
        const codex::Notification wire{
            "command/exec/outputDelta",
            params,
            raw,
        };

        const typed::Event event = detail::decodeEvent(wire);
        const auto* decoded = std::get_if<typed::CommandExecOutputDeltaNotification>(&event);
        result.expectTrue(decoded && decoded->raw == raw && decoded->deltaBase64 == SyntheticDelta &&
                              decoded->processId.value == SyntheticProcessId && decoded->capReached &&
                              decoded->stream == typed::CommandExecOutputStream::stderrStream(),
                          "typed command output retains every stable field and the raw envelope internally");

        backend::Reducer reducer;
        backend::BackendState state;
        const backend::Snapshot before = backend::makeSnapshot(state);
        const std::vector<backend::BackendEvent> translated = reducer.translate(event);
        const auto* extension = translated.size() == 1 ? std::get_if<backend::CodexExtensionReceived>(&translated.front()) : nullptr;
        result.expectTrue(extension && extension->method == "command/exec/outputDelta" && extension->payload == params &&
                              !extension->payload.contains("jsonrpc") && !extension->payload.contains("method") &&
                              !extension->payload.contains("futureEnvelopeOnly"),
                          "command output reuses the existing params-only extension path");

        if (extension) {
            const backend::Reduction reduction = reducer.apply(state, *extension);
            result.expectTrue(reduction.changed && !reduction.flushImmediately,
                              "command output changes only the existing bounded extension history");
        }

        const backend::ExtensionRecord* retained = state.recentExtensions.size() == 1 ? &state.recentExtensions.front() : nullptr;
        result.expectTrue(retained && retained->method == "command/exec/outputDelta" && retained->payload == params,
                          "bounded internal extension state retains exact command params before projection");

        const backend::Snapshot snapshot = backend::makeSnapshot(state);
        const backend::ExtensionSnapshot* projected = snapshot.recentExtensions.size() == 1 ? &snapshot.recentExtensions.front() : nullptr;
        const codex::Json frontendData = projected ? frontendCompatibleData(*projected) : codex::Json::object();
        const std::string frontendBytes = frontendData.dump();
        result.expectTrue(
            projected && projected->method == "command/exec/outputDelta" && projected->sensitiveFieldsRedacted &&
                projected->payload.value("deltaBase64", "") == "[redacted]" && projected->payload.value("processId", "") == "[redacted]" &&
                projected->payload.value("stream", "") == "stderr" && projected->payload.value("capReached", false) &&
                projected->payload.value("futureSafeField", "") == "visible" && frontendData.size() == 3 &&
                frontendData.value("sensitiveFieldsRedacted", false) && frontendBytes.find(SyntheticDelta) == std::string::npos &&
                frontendBytes.find(SyntheticProcessId) == std::string::npos &&
                frontendBytes.find("futureEnvelopeOnly") == std::string::npos,
            "frontend-compatible extension bytes redact encoded output and process identity "
            "while preserving the existing generic event shape");

        result.expectTrue(withoutExtensions(snapshot) == before && state.threads.empty() && state.threadOrder.empty() &&
                              state.pendingRequests.empty(),
                          "command output adds no command state, thread item, or approval state");
    }

    void testRedactionIsMethodSpecific(tests::support::TestResult& result) {
        const backend::ExtensionSnapshot unrelated = backend::makeExtensionSnapshot({
            .method = "future/extension",
            .payload =
                {
                    {"deltaBase64", "ordinary-future-value"},
                    {"processId", "ordinary-future-id"},
                },
        });
        result.expectTrue(!unrelated.sensitiveFieldsRedacted && unrelated.payload.value("deltaBase64", "") == "ordinary-future-value" &&
                              unrelated.payload.value("processId", "") == "ordinary-future-id",
                          "command redaction does not broaden generic extension key semantics");
    }
} // namespace

int main() {
    static_assert(std::is_constructible_v<typed::Event, typed::CommandExecOutputDeltaNotification>);

    tests::support::TestResult result;
    testCommandOutputUsesExistingRedactedExtensionBoundary(result);
    testRedactionIsMethodSpecific(result);
    return result.processResult();
}
