/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/backend/Reducer.h"
#include "ai/openai/codex/backend/Snapshot.h"
#include "ai/openai/codex/detail/EventDecoder.h"
#include "ai/openai/codex/typed/Events.h"
#include "ai/openai/codex/typed/Filesystem.h"
#include "support/TestResult.h"

#include <array>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    struct Case {
        const char* method;
        codex::Json params;
    };

    codex::Notification notification(const Case& testCase) {
        return {
            testCase.method,
            testCase.params,
            {
                {"jsonrpc", "2.0"},
                {"method", testCase.method},
                {"params", testCase.params},
            },
        };
    }

    bool isExpectedAlternative(std::string_view method, const typed::Event& event) {
        if (method == "fs/changed") {
            return std::holds_alternative<typed::FsChangedNotification>(event);
        }
        if (method == "fuzzyFileSearch/sessionCompleted") {
            return std::holds_alternative<typed::FuzzyFileSearchSessionCompletedNotification>(event);
        }
        return std::holds_alternative<typed::FuzzyFileSearchSessionUpdatedNotification>(event);
    }

    void testExistingExtensionBoundaryAndRedaction(tests::support::TestResult& result) {
        const std::array<Case, 3> cases{{
            {
                "fs/changed",
                {
                    {"changedPaths", codex::Json::array({"/synthetic/private/a", "/synthetic/private/b"})},
                    {"watchId", "synthetic-private-watch"},
                    {"futureSafeField", true},
                },
            },
            {
                "fuzzyFileSearch/sessionCompleted",
                {
                    {"sessionId", "synthetic-private-session"},
                    {"futureSafeField", true},
                },
            },
            {
                "fuzzyFileSearch/sessionUpdated",
                {
                    {"files",
                     codex::Json::array({
                         {
                             {"file_name", "synthetic-secret-name"},
                             {"indices", codex::Json::array({0})},
                             {"match_type", "file"},
                             {"path", "/synthetic/private/result"},
                             {"root", "/synthetic/private"},
                             {"score", 99},
                         },
                     })},
                    {"query", "synthetic-private-query"},
                    {"sessionId", "synthetic-private-session"},
                    {"futureSafeField", true},
                },
            },
        }};

        backend::Reducer reducer;
        backend::BackendState state;
        const backend::Snapshot before = backend::makeSnapshot(state);

        for (const Case& testCase : cases) {
            const typed::Event event = detail::decodeEvent(notification(testCase));
            result.expectTrue(isExpectedAlternative(testCase.method, event),
                              std::string(testCase.method) + " decodes through its appended typed Events alternative");

            const std::vector<backend::BackendEvent> translated = reducer.translate(event);
            const auto* extension = translated.size() == 1 ? std::get_if<backend::CodexExtensionReceived>(&translated.front()) : nullptr;
            result.expectTrue(extension && extension->method == testCase.method && extension->payload == testCase.params,
                              std::string(testCase.method) + " preserves exact params internally through the existing extension event");
            if (extension != nullptr) {
                reducer.apply(state, *extension);
            }
        }

        const backend::Snapshot snapshot = backend::makeSnapshot(state);
        result.expectTrue(snapshot.recentExtensions.size() == cases.size(),
                          "all typed filesystem notifications retain existing bounded observer ordering");

        if (snapshot.recentExtensions.size() == cases.size()) {
            const backend::ExtensionSnapshot& changed = snapshot.recentExtensions[0];
            const backend::ExtensionSnapshot& completed = snapshot.recentExtensions[1];
            const backend::ExtensionSnapshot& updated = snapshot.recentExtensions[2];

            result.expectTrue(changed.sensitiveFieldsRedacted && changed.payload.value("changedPaths", "") == "[redacted]" &&
                                  changed.payload.value("watchId", "") == "[redacted]" && changed.payload.value("futureSafeField", false),
                              "frontend projection redacts changed paths and watch identity");
            result.expectTrue(completed.sensitiveFieldsRedacted && completed.payload.value("sessionId", "") == "[redacted]" &&
                                  completed.payload.value("futureSafeField", false),
                              "frontend projection redacts fuzzy session correlation");
            result.expectTrue(updated.sensitiveFieldsRedacted && updated.payload.value("files", "") == "[redacted]" &&
                                  updated.payload.value("query", "") == "[redacted]" &&
                                  updated.payload.value("sessionId", "") == "[redacted]" && updated.payload.value("futureSafeField", false),
                              "frontend projection redacts fuzzy paths, listing data, query, and session identity");

            const std::string frontendBytes =
                codex::Json{
                    {"changed", changed.payload},
                    {"completed", completed.payload},
                    {"updated", updated.payload},
                }
                    .dump();
            result.expectTrue(frontendBytes.find("/synthetic/private") == std::string::npos &&
                                  frontendBytes.find("synthetic-secret-name") == std::string::npos &&
                                  frontendBytes.find("synthetic-private-query") == std::string::npos &&
                                  frontendBytes.find("synthetic-private-watch") == std::string::npos &&
                                  frontendBytes.find("synthetic-private-session") == std::string::npos,
                              "frontend-compatible bytes contain no synthetic filesystem-sensitive values");
        }

        backend::Snapshot withoutExtensions = snapshot;
        withoutExtensions.recentExtensions.clear();
        withoutExtensions.omittedRecentExtensions = 0;
        result.expectTrue(withoutExtensions == before && state.threads.empty() && state.threadOrder.empty() &&
                              state.pendingRequests.empty(),
                          "filesystem notifications add no BackendState watcher, filesystem, or fuzzy-search model");
    }

    void testRedactionIsMethodSpecific(tests::support::TestResult& result) {
        const backend::ExtensionSnapshot unrelated = backend::makeExtensionSnapshot({
            .method = "future/extension",
            .payload =
                {
                    {"changedPaths", "ordinary-value"},
                    {"files", "ordinary-files"},
                    {"query", "ordinary-query"},
                    {"sessionId", "ordinary-session"},
                    {"watchId", "ordinary-watch"},
                },
        });
        result.expectTrue(!unrelated.sensitiveFieldsRedacted && unrelated.payload.value("changedPaths", "") == "ordinary-value" &&
                              unrelated.payload.value("files", "") == "ordinary-files" &&
                              unrelated.payload.value("query", "") == "ordinary-query" &&
                              unrelated.payload.value("sessionId", "") == "ordinary-session" &&
                              unrelated.payload.value("watchId", "") == "ordinary-watch",
                          "filesystem redaction is narrowly scoped to the three pinned notification methods");
    }
} // namespace

int main() {
    tests::support::TestResult result;
    testExistingExtensionBoundaryAndRedaction(result);
    testRedactionIsMethodSpecific(result);
    return result.processResult();
}
