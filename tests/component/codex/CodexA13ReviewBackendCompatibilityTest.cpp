/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/backend/BackendCommand.h"
#include "ai/openai/codex/backend/Reducer.h"
#include "ai/openai/codex/backend/Snapshot.h"
#include "ai/openai/codex/detail/EventDecoder.h"
#include "ai/openai/codex/frontend/Codec.h"
#include "ai/openai/codex/typed/Events.h"
#include "ai/openai/codex/typed/Reviews.h"
#include "support/TestResult.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace {
    namespace backend = ai::openai::codex::backend;
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace frontend = ai::openai::codex::frontend;
    namespace typed = ai::openai::codex::typed;

    constexpr const char* WarningMessage = "synthetic-private-guardian-warning";
    constexpr const char* CommandValue = "synthetic-private-guardian-command";
    constexpr const char* CommandCwd = "/synthetic/private/guardian-cwd";
    constexpr const char* NetworkTarget = "https://synthetic.private/resource";
    constexpr const char* ReviewRationale = "synthetic-private-review-rationale";
    constexpr const char* TargetItem = "synthetic-private-target-item";
    constexpr const char* ThreadIdentity = "synthetic-private-thread";
    constexpr const char* TurnIdentity = "synthetic-private-turn";
    constexpr const char* ReviewIdentity = "synthetic-private-review";

    template <typename T>
    concept HasGuardianReviewState = requires(T value) { value.guardianReviews; };

    template <typename T>
    concept HasReviewResultState = requires(T value) { value.reviewResults; };

    template <typename T>
    concept HasGuardianPolicy = requires(T value) { value.guardianPolicy; };

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
                {"futureEnvelopeOnly", "must-not-reach-frontend"},
            },
        };
    }

    bool isExpectedAlternative(std::string_view method, const typed::Event& event) {
        if (method == "guardianWarning") {
            return std::holds_alternative<typed::GuardianWarningNotification>(event);
        }
        if (method == "item/autoApprovalReview/started") {
            return std::holds_alternative<typed::ItemGuardianApprovalReviewStartedNotification>(event);
        }
        return std::holds_alternative<typed::ItemGuardianApprovalReviewCompletedNotification>(event);
    }

    codex::Json frontendExtensionData(const backend::ExtensionSnapshot& extension) {
        codex::Json data{
            {"method", extension.method},
            {"params", extension.payload},
        };
        if (extension.sensitiveFieldsRedacted) {
            data["sensitiveFieldsRedacted"] = true;
        }
        return data;
    }

    void testGenericRedactedFrontendBoundary(tests::support::TestResult& result) {
        const std::array<Case, 3> cases{{
            {
                "guardianWarning",
                {
                    {"message", WarningMessage},
                    {"threadId", ThreadIdentity},
                    {"futureSafeField", "warning-safe"},
                },
            },
            {
                "item/autoApprovalReview/started",
                {
                    {"action",
                     {
                         {"command", CommandValue},
                         {"cwd", CommandCwd},
                         {"source", "shell"},
                         {"type", "command"},
                     }},
                    {"review",
                     {
                         {"rationale", ReviewRationale},
                         {"riskLevel", "high"},
                         {"status", "inProgress"},
                         {"userAuthorization", "medium"},
                     }},
                    {"reviewId", ReviewIdentity},
                    {"startedAtMs", 101},
                    {"targetItemId", TargetItem},
                    {"threadId", ThreadIdentity},
                    {"turnId", TurnIdentity},
                    {"futureSafeField", "started-safe"},
                },
            },
            {
                "item/autoApprovalReview/completed",
                {
                    {"action",
                     {
                         {"host", "synthetic.private"},
                         {"port", 443},
                         {"protocol", "https"},
                         {"target", NetworkTarget},
                         {"type", "networkAccess"},
                     }},
                    {"completedAtMs", 202},
                    {"decisionSource", "agent"},
                    {"review",
                     {
                         {"rationale", ReviewRationale},
                         {"riskLevel", "critical"},
                         {"status", "denied"},
                         {"userAuthorization", "low"},
                     }},
                    {"reviewId", ReviewIdentity},
                    {"startedAtMs", 102},
                    {"targetItemId", nullptr},
                    {"threadId", ThreadIdentity},
                    {"turnId", TurnIdentity},
                    {"futureSafeField", "completed-safe"},
                },
            },
        }};

        backend::Reducer reducer;
        backend::BackendState state;
        const backend::Snapshot before = backend::makeSnapshot(state);

        for (const Case& testCase : cases) {
            const typed::Event event = detail::decodeEvent(notification(testCase));
            result.expectTrue(isExpectedAlternative(testCase.method, event),
                              std::string(testCase.method) + " decodes through its appended typed Event alternative");
            const std::vector<backend::BackendEvent> translated = reducer.translate(event);
            const auto* extension = translated.size() == 1 ? std::get_if<backend::CodexExtensionReceived>(&translated.front()) : nullptr;
            result.expectTrue(extension && extension->method == testCase.method && extension->payload == testCase.params,
                              std::string(testCase.method) + " preserves exact params through the existing extension event");
            if (extension != nullptr) {
                const backend::Reduction reduction = reducer.apply(state, *extension);
                result.expectTrue(reduction.changed && !reduction.flushImmediately,
                                  std::string(testCase.method) + " changes only bounded extension history");
            }
        }

        const backend::Snapshot snapshot = backend::makeSnapshot(state);
        result.expectTrue(snapshot.recentExtensions.size() == cases.size(),
                          "all three review notifications retain existing observer ordering");
        if (snapshot.recentExtensions.size() != cases.size()) {
            return;
        }

        const backend::ExtensionSnapshot& warning = snapshot.recentExtensions[0];
        const backend::ExtensionSnapshot& started = snapshot.recentExtensions[1];
        const backend::ExtensionSnapshot& completed = snapshot.recentExtensions[2];
        result.expectTrue(warning.method == "guardianWarning" && warning.sensitiveFieldsRedacted &&
                              warning.payload.value("message", "") == "[redacted]" &&
                              warning.payload.value("threadId", "") == "[redacted]" &&
                              warning.payload.value("futureSafeField", "") == "warning-safe",
                          "guardian warning keeps its generic shape while redacting details and identity");
        result.expectTrue(
            started.method == "item/autoApprovalReview/started" && started.sensitiveFieldsRedacted &&
                started.payload.value("action", "") == "[redacted]" && started.payload.value("review", "") == "[redacted]" &&
                started.payload.value("reviewId", "") == "[redacted]" && started.payload.value("targetItemId", "") == "[redacted]" &&
                started.payload.value("threadId", "") == "[redacted]" && started.payload.value("turnId", "") == "[redacted]" &&
                started.payload.value("startedAtMs", 0) == 101 && started.payload.value("futureSafeField", "") == "started-safe",
            "started review keeps timestamps and safe extensions while redacting "
            "action and review data");
        result.expectTrue(
            completed.method == "item/autoApprovalReview/completed" && completed.sensitiveFieldsRedacted &&
                completed.payload.value("action", "") == "[redacted]" && completed.payload.value("review", "") == "[redacted]" &&
                completed.payload.value("reviewId", "") == "[redacted]" && completed.payload.value("targetItemId", "") == "[redacted]" &&
                completed.payload.value("threadId", "") == "[redacted]" && completed.payload.value("turnId", "") == "[redacted]" &&
                completed.payload.value("startedAtMs", 0) == 102 && completed.payload.value("completedAtMs", 0) == 202 &&
                completed.payload.value("decisionSource", "") == "agent" &&
                completed.payload.value("futureSafeField", "") == "completed-safe",
            "completed review keeps lifecycle metadata while redacting action and review data");

        std::vector<codex::Json> frontendEvents;
        frontendEvents.reserve(snapshot.recentExtensions.size());
        for (std::size_t index = 0; index < snapshot.recentExtensions.size(); ++index) {
            const backend::ExtensionSnapshot& extension = snapshot.recentExtensions[index];
            const frontend::FrontendEvent event{
                frontend::SequenceNumber{static_cast<std::uint64_t>(index + 1)},
                "codex.extension",
                frontendExtensionData(extension),
                codex::Json::object(),
            };
            const auto encoded = frontend::Codec::encodeEvent(event);
            result.expectTrue(encoded.hasValue() && encoded.value() ==
                                                        codex::Json{
                                                            {"data", frontendExtensionData(extension)},
                                                            {"sequence", static_cast<std::uint64_t>(index + 1)},
                                                            {"type", "codex.extension"},
                                                        },
                              "review notification retains the exact existing frontend "
                              "codex.extension bytes");
            if (encoded) {
                frontendEvents.push_back(encoded.value());
            }
        }
        const std::string frontendBytes = codex::Json(frontendEvents).dump();
        result.expectTrue(
            frontendBytes.find(WarningMessage) == std::string::npos && frontendBytes.find(CommandValue) == std::string::npos &&
                frontendBytes.find(CommandCwd) == std::string::npos && frontendBytes.find(NetworkTarget) == std::string::npos &&
                frontendBytes.find(ReviewRationale) == std::string::npos && frontendBytes.find(TargetItem) == std::string::npos &&
                frontendBytes.find(ThreadIdentity) == std::string::npos && frontendBytes.find(TurnIdentity) == std::string::npos &&
                frontendBytes.find(ReviewIdentity) == std::string::npos &&
                frontendBytes.find("must-not-reach-frontend") == std::string::npos,
            "frontend-compatible bytes contain no synthetic guardian-sensitive "
            "values or envelopes");

        backend::Snapshot withoutExtensions = snapshot;
        withoutExtensions.recentExtensions.clear();
        withoutExtensions.omittedRecentExtensions = 0;
        result.expectTrue(withoutExtensions == before && state.threads.empty() && state.threadOrder.empty() &&
                              state.pendingRequests.empty(),
                          "guardian notifications add no semantic review, thread, or approval state");
    }

    void testRedactionIsMethodSpecific(tests::support::TestResult& result) {
        const backend::ExtensionSnapshot unrelated = backend::makeExtensionSnapshot({
            .method = "future/extension",
            .payload =
                {
                    {"action", "ordinary-action"},
                    {"message", "ordinary-message"},
                    {"review", "ordinary-review"},
                    {"reviewId", "ordinary-review-id"},
                    {"targetItemId", "ordinary-item"},
                    {"threadId", "ordinary-thread"},
                    {"turnId", "ordinary-turn"},
                },
        });
        result.expectTrue(!unrelated.sensitiveFieldsRedacted && unrelated.payload.value("action", "") == "ordinary-action" &&
                              unrelated.payload.value("message", "") == "ordinary-message" &&
                              unrelated.payload.value("review", "") == "ordinary-review" &&
                              unrelated.payload.value("reviewId", "") == "ordinary-review-id" &&
                              unrelated.payload.value("targetItemId", "") == "ordinary-item" &&
                              unrelated.payload.value("threadId", "") == "ordinary-thread" &&
                              unrelated.payload.value("turnId", "") == "ordinary-turn",
                          "guardian redaction is narrowly scoped to the three pinned notification methods");
    }
} // namespace

int main() {
    static_assert(std::variant_size_v<backend::BackendCommand> == 15);
    static_assert(std::is_same_v<std::variant_alternative_t<0, backend::BackendCommand>, backend::ControllerAcquire>);
    static_assert(std::is_same_v<std::variant_alternative_t<14, backend::BackendCommand>, backend::UnknownRequestReject>);
    static_assert(!HasGuardianReviewState<backend::BackendState>);
    static_assert(!HasReviewResultState<backend::BackendState>);
    static_assert(!HasGuardianPolicy<backend::BackendState>);

    tests::support::TestResult result;
    testGenericRedactedFrontendBoundary(result);
    testRedactionIsMethodSpecific(result);
    return result.processResult();
}
