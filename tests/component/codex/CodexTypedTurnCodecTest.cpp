/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/TurnCodec.h"
#include "support/TestResult.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace {
    using ai::openai::codex::Json;
    using ai::openai::codex::detail::decodeTurn;
    using ai::openai::codex::detail::decodeTurnInterruptResult;
    using ai::openai::codex::detail::decodeTurnStartResult;
    using ai::openai::codex::detail::encodeTurnInput;
    using ai::openai::codex::detail::encodeTurnInterruptParams;
    using ai::openai::codex::detail::encodeTurnStartParams;
    using ai::openai::codex::typed::ApprovalPolicy;
    using ai::openai::codex::typed::DangerFullAccessSandboxPolicy;
    using ai::openai::codex::typed::ExternalSandboxPolicy;
    using ai::openai::codex::typed::ImageDetail;
    using ai::openai::codex::typed::ImageUrlInput;
    using ai::openai::codex::typed::LocalImageInput;
    using ai::openai::codex::typed::MentionInput;
    using ai::openai::codex::typed::ModelId;
    using ai::openai::codex::typed::NetworkAccess;
    using ai::openai::codex::typed::ReadOnlySandboxPolicy;
    using ai::openai::codex::typed::ReasoningEffort;
    using ai::openai::codex::typed::SkillInput;
    using ai::openai::codex::typed::TextInput;
    using ai::openai::codex::typed::ThreadId;
    using ai::openai::codex::typed::Turn;
    using ai::openai::codex::typed::TurnId;
    using ai::openai::codex::typed::TurnInput;
    using ai::openai::codex::typed::TurnStartOptions;
    using ai::openai::codex::typed::UnknownTurnInput;
    using ai::openai::codex::typed::WorkspaceWriteSandboxPolicy;

    Json validTurn(std::string status = "completed") {
        return {
            {"id", "turn-1"},
            {"items", Json::array()},
            {"status", std::move(status)},
            {"error", nullptr},
            {"startedAt", 1'700'000'010},
            {"completedAt", 1'700'000'015},
            {"durationMs", 5'000},
            {"futureTurnField", Json{{"kept", true}}},
        };
    }

    void testValidTurnAndRawPreservation(tests::support::TestResult& testResult) {
        const Json raw = validTurn();
        std::string error = "stale";
        const std::optional<Turn> decoded = decodeTurn(raw, ThreadId{"thread-1"}, error);

        testResult.expectTrue(decoded.has_value(), "current-schema turn is decoded");
        testResult.expectTrue(error.empty(), "successful turn decoding clears a previous error");
        testResult.expectTrue(decoded && decoded->id.value == "turn-1" && decoded->threadId.value == "thread-1",
                              "turn and parent thread strong IDs are preserved without interchange");
        testResult.expectTrue(decoded && decoded->status.value == "completed" && decoded->itemsView.value == "full",
                              "turn status and default full items view are decoded");
        testResult.expectTrue(decoded && decoded->items.empty(), "empty current-schema turn item list is accepted");
        testResult.expectTrue(decoded && !decoded->error && decoded->startedAt == 1'700'000'010 && decoded->completedAt == 1'700'000'015 &&
                                  decoded->durationMs == 5'000,
                              "turn optional error and timing fields decode with null safety");
        testResult.expectTrue(decoded && decoded->raw == raw, "turn raw JSON preserves unknown fields and the complete original object");
    }

    void testRequiredOptionalAndExtensibleFields(tests::support::TestResult& testResult) {
        std::string error;

        Json missingId = validTurn();
        missingId.erase("id");
        testResult.expectTrue(!decodeTurn(missingId, ThreadId{"thread-1"}, error) && !error.empty(),
                              "turn missing its required ID is rejected");

        Json invalidId = validTurn();
        invalidId["id"] = Json::object();
        testResult.expectTrue(!decodeTurn(invalidId, ThreadId{"thread-1"}, error), "turn with a non-string ID is rejected");

        Json missingStatus = validTurn();
        missingStatus.erase("status");
        testResult.expectTrue(!decodeTurn(missingStatus, ThreadId{"thread-1"}, error), "turn missing required status is rejected");

        Json missingItems = validTurn();
        missingItems.erase("items");
        testResult.expectTrue(!decodeTurn(missingItems, ThreadId{"thread-1"}, error), "turn missing required items is rejected");

        Json optionalAbsent = validTurn();
        optionalAbsent.erase("error");
        optionalAbsent.erase("startedAt");
        optionalAbsent.erase("completedAt");
        optionalAbsent.erase("durationMs");
        const std::optional<Turn> decodedAbsent = decodeTurn(optionalAbsent, ThreadId{"thread-1"}, error);
        testResult.expectTrue(decodedAbsent && !decodedAbsent->error && !decodedAbsent->startedAt && !decodedAbsent->completedAt &&
                                  !decodedAbsent->durationMs,
                              "absent optional turn fields remain absent");

        Json optionalNull = optionalAbsent;
        optionalNull["error"] = nullptr;
        optionalNull["startedAt"] = nullptr;
        optionalNull["completedAt"] = nullptr;
        optionalNull["durationMs"] = nullptr;
        const std::optional<Turn> decodedNull = decodeTurn(optionalNull, ThreadId{"thread-1"}, error);
        testResult.expectTrue(decodedNull && !decodedNull->error && !decodedNull->startedAt && !decodedNull->completedAt &&
                                  !decodedNull->durationMs,
                              "null optional turn fields decode safely as absent");

        Json failed = validTurn("failed");
        failed["error"] = Json{{"message", "remote turn failure"}, {"additionalDetails", nullptr}, {"futureErrorField", 7}};
        const std::optional<Turn> decodedFailure = decodeTurn(failed, ThreadId{"thread-1"}, error);
        testResult.expectTrue(decodedFailure && decodedFailure->error && *decodedFailure->error == failed.at("error"),
                              "typed turn retains the complete current-schema error object and unknown error fields");

        Json malformedError = failed;
        malformedError["error"].erase("message");
        testResult.expectTrue(!decodeTurn(malformedError, ThreadId{"thread-1"}, error),
                              "turn error missing its required message is rejected");

        const Json unknownRaw = validTurn("futureTurnStatus");
        const std::optional<Turn> unknown = decodeTurn(unknownRaw, ThreadId{"thread-1"}, error);
        testResult.expectTrue(unknown && unknown->status.value == "futureTurnStatus" && unknown->raw == unknownRaw,
                              "unknown future turn status strings remain nonfatal and raw-preserved");

        Json futureItemsView = validTurn();
        futureItemsView["itemsView"] = "futureView";
        const std::optional<Turn> decodedFutureItemsView = decodeTurn(futureItemsView, ThreadId{"thread-1"}, error);
        testResult.expectTrue(decodedFutureItemsView && decodedFutureItemsView->itemsView.value == "futureView",
                              "unknown future turn items view strings remain nonfatal and raw-preserved");

        Json overflowTimestamp = validTurn();
        overflowTimestamp["startedAt"] = std::numeric_limits<std::uint64_t>::max();
        testResult.expectTrue(!decodeTurn(overflowTimestamp, ThreadId{"thread-1"}, error),
                              "turn timestamp outside the schema int64 range is rejected safely");

        Json negativeDuration = validTurn();
        negativeDuration["durationMs"] = -1;
        const std::optional<Turn> decodedNegativeDuration = decodeTurn(negativeDuration, ThreadId{"thread-1"}, error);
        testResult.expectTrue(decodedNegativeDuration && decodedNegativeDuration->durationMs == -1,
                              "signed int64 duration is preserved exactly without inventing a schema minimum");

        Json overflowDuration = validTurn();
        overflowDuration["durationMs"] = std::numeric_limits<std::uint64_t>::max();
        testResult.expectTrue(!decodeTurn(overflowDuration, ThreadId{"thread-1"}, error),
                              "turn duration outside the schema int64 range is rejected safely");
    }

    void testInputEncoding(tests::support::TestResult& testResult) {
        std::string error = "stale";

        const std::optional<Json> text = encodeTurnInput(TextInput{"Analyse the branch."}, error);
        const Json expectedText = {
            {"type", "text"},
            {"text", "Analyse the branch."},
            {"text_elements", Json::array()},
        };
        testResult.expectTrue(text && *text == expectedText,
                              "text input uses the exact discriminator and emits the required empty text_elements array");
        testResult.expectTrue(error.empty(), "successful text input encoding clears a previous error");

        const std::optional<Json> image = encodeTurnInput(ImageUrlInput{"https://example.test/image.png", ImageDetail{"high"}}, error);
        testResult.expectTrue(image && *image == Json{{"type", "image"}, {"url", "https://example.test/image.png"}, {"detail", "high"}},
                              "image URL input uses the current 'image' discriminator and detail field");

        const std::optional<Json> localImage = encodeTurnInput(LocalImageInput{"/tmp/image.png", std::nullopt}, error);
        testResult.expectTrue(localImage && *localImage == Json{{"type", "localImage"}, {"path", "/tmp/image.png"}},
                              "local image input uses the current camel-case discriminator and omits absent detail");

        const std::optional<Json> skill = encodeTurnInput(SkillInput{"review", "/tmp/SKILL.md"}, error);
        testResult.expectTrue(skill && *skill == Json{{"type", "skill"}, {"name", "review"}, {"path", "/tmp/SKILL.md"}},
                              "skill input encodes its exact current-schema shape");

        const std::optional<Json> mention = encodeTurnInput(MentionInput{"README", "/tmp/README.md"}, error);
        testResult.expectTrue(mention && *mention == Json{{"type", "mention"}, {"name", "README"}, {"path", "/tmp/README.md"}},
                              "mention input encodes its exact current-schema shape");

        const UnknownTurnInput futureInput{"futureInput", Json{{"type", "futureInput"}, {"payload", true}}};
        const std::optional<Json> unknown = encodeTurnInput(futureInput, error);
        testResult.expectTrue(!unknown && !error.empty(),
                              "UnknownTurnInput is rejected locally instead of guessing a future outgoing wire shape");
    }

    void testTurnStartEncoding(tests::support::TestResult& testResult) {
        const std::vector<TurnInput> inputs = {
            TextInput{"first"},
            ImageUrlInput{"https://example.test/image.png", ImageDetail{"original"}},
            LocalImageInput{"/tmp/image.png", ImageDetail{"low"}},
            SkillInput{"skill", "/tmp/SKILL.md"},
            MentionInput{"file", "/tmp/file.cpp"},
        };
        TurnStartOptions options;
        options.cwd = "/tmp/project";
        options.model = ModelId{"gpt-5.4"};
        options.reasoningEffort = ReasoningEffort{"xhigh"};
        options.approvalPolicy = ApprovalPolicy{"on-request"};
        options.sandboxPolicy = WorkspaceWriteSandboxPolicy{{"/tmp/project"}, true, true, false};

        std::string error;
        const std::optional<Json> encoded = encodeTurnStartParams(ThreadId{"thread-1"}, inputs, options, error);
        const Json expectedInput = Json::array({
            Json{{"type", "text"}, {"text", "first"}, {"text_elements", Json::array()}},
            Json{{"type", "image"}, {"url", "https://example.test/image.png"}, {"detail", "original"}},
            Json{{"type", "localImage"}, {"path", "/tmp/image.png"}, {"detail", "low"}},
            Json{{"type", "skill"}, {"name", "skill"}, {"path", "/tmp/SKILL.md"}},
            Json{{"type", "mention"}, {"name", "file"}, {"path", "/tmp/file.cpp"}},
        });
        const Json expected = {
            {"threadId", "thread-1"},
            {"input", expectedInput},
            {"cwd", "/tmp/project"},
            {"model", "gpt-5.4"},
            {"effort", "xhigh"},
            {"approvalPolicy", "on-request"},
            {"sandboxPolicy",
             {{"type", "workspaceWrite"},
              {"writableRoots", Json::array({"/tmp/project"})},
              {"networkAccess", true},
              {"excludeTmpdirEnvVar", true},
              {"excludeSlashTmp", false}}},
        };
        testResult.expectTrue(encoded && *encoded == expected,
                              "turn/start encodes multiple typed inputs, strong thread ID, and every supported optional setting");

        const std::optional<Json> minimum = encodeTurnStartParams(ThreadId{"thread-minimum"}, {}, {}, error);
        testResult.expectTrue(minimum && *minimum == Json{{"threadId", "thread-minimum"}, {"input", Json::array()}},
                              "turn/start omits absent settings and accepts the schema's empty input array");

        TurnStartOptions invalidEffort;
        invalidEffort.reasoningEffort = ReasoningEffort{""};
        testResult.expectTrue(!encodeTurnStartParams(ThreadId{"thread-1"}, {}, invalidEffort, error) && !error.empty(),
                              "turn/start rejects an empty reasoning effort prohibited by the current schema");

        const std::vector<TurnInput> unknownInputs = {
            TextInput{"before"},
            UnknownTurnInput{"future", Json{{"type", "future"}}},
        };
        testResult.expectTrue(!encodeTurnStartParams(ThreadId{"thread-1"}, unknownInputs, {}, error) && !error.empty(),
                              "turn/start rejects a mixed input list containing an unknown outgoing variant");

        TurnStartOptions readOnly;
        readOnly.sandboxPolicy = ReadOnlySandboxPolicy{false};
        const auto encodedReadOnly = encodeTurnStartParams(ThreadId{"thread-1"}, {}, readOnly, error);
        testResult.expectTrue(encodedReadOnly &&
                                  (*encodedReadOnly)["sandboxPolicy"] == Json{{"type", "readOnly"}, {"networkAccess", false}},
                              "turn/start encodes the current read-only sandbox policy shape");

        TurnStartOptions external;
        external.sandboxPolicy = ExternalSandboxPolicy{NetworkAccess::enabled()};
        const auto encodedExternal = encodeTurnStartParams(ThreadId{"thread-1"}, {}, external, error);
        testResult.expectTrue(encodedExternal &&
                                  (*encodedExternal)["sandboxPolicy"] == Json{{"type", "externalSandbox"}, {"networkAccess", "enabled"}},
                              "turn/start encodes the current external sandbox policy shape");

        TurnStartOptions fullAccess;
        fullAccess.sandboxPolicy = DangerFullAccessSandboxPolicy{};
        const auto encodedFullAccess = encodeTurnStartParams(ThreadId{"thread-1"}, {}, fullAccess, error);
        testResult.expectTrue(encodedFullAccess && (*encodedFullAccess)["sandboxPolicy"] == Json{{"type", "dangerFullAccess"}},
                              "turn/start encodes the current danger-full-access sandbox policy shape");
    }

    void testResultWrappersAndInterrupt(tests::support::TestResult& testResult) {
        const Json rawTurn = validTurn();
        const Json rawResult = {{"turn", rawTurn}, {"futureResultField", true}};
        std::string error;
        const std::optional<Turn> decoded = decodeTurnStartResult(rawResult, ThreadId{"thread-result"}, error);
        testResult.expectTrue(decoded && decoded->id.value == "turn-1" && decoded->threadId.value == "thread-result",
                              "turn/start wrapper decodes the typed turn with the request's parent thread ID");
        testResult.expectTrue(decoded && decoded->raw == rawTurn, "turn/start typed value preserves the exact nested raw turn object");

        testResult.expectTrue(!decodeTurnStartResult(Json::object(), ThreadId{"thread-result"}, error) && !error.empty(),
                              "turn/start wrapper missing its required turn is rejected as a typed decoding failure");
        testResult.expectTrue(!decodeTurnStartResult(Json{{"turn", Json::array()}}, ThreadId{"thread-result"}, error),
                              "turn/start wrapper with a non-object turn is rejected");

        Json malformedNested = rawResult;
        malformedNested["turn"].erase("status");
        testResult.expectTrue(!decodeTurnStartResult(malformedNested, ThreadId{"thread-result"}, error),
                              "turn/start wrapper with a malformed nested typed turn is rejected");

        const std::optional<Json> interruptParams =
            encodeTurnInterruptParams(ThreadId{"thread-interrupt"}, TurnId{"turn-interrupt"}, error);
        testResult.expectTrue(interruptParams && *interruptParams == Json{{"threadId", "thread-interrupt"}, {"turnId", "turn-interrupt"}},
                              "turn/interrupt encodes non-interchangeable thread and turn IDs with exact field names");
        testResult.expectTrue(decodeTurnInterruptResult(Json::object(), error).has_value(),
                              "empty current-schema turn/interrupt response object decodes successfully");
        testResult.expectTrue(!decodeTurnInterruptResult(nullptr, error) && !error.empty(),
                              "malformed non-object turn/interrupt response is rejected");
    }
} // namespace

int main() {
    tests::support::TestResult testResult;

    testValidTurnAndRawPreservation(testResult);
    testRequiredOptionalAndExtensibleFields(testResult);
    testInputEncoding(testResult);
    testTurnStartEncoding(testResult);
    testResultWrappersAndInterrupt(testResult);

    return testResult.processResult();
}
