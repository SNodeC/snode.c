/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/detail/ThreadCodec.h"
#include "support/TestResult.h"

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

namespace {
    using ai::openai::codex::Json;
    using ai::openai::codex::detail::decodeThread;
    using ai::openai::codex::detail::decodeThreadListResult;
    using ai::openai::codex::detail::decodeThreadOperationResult;
    using ai::openai::codex::detail::decodeThreadReadResult;
    using ai::openai::codex::detail::encodeThreadListParams;
    using ai::openai::codex::detail::encodeThreadReadParams;
    using ai::openai::codex::detail::encodeThreadResumeParams;
    using ai::openai::codex::detail::encodeThreadStartParams;
    using ai::openai::codex::typed::ApprovalPolicy;
    using ai::openai::codex::typed::ModelId;
    using ai::openai::codex::typed::SandboxMode;
    using ai::openai::codex::typed::Thread;
    using ai::openai::codex::typed::ThreadId;
    using ai::openai::codex::typed::ThreadListOptions;
    using ai::openai::codex::typed::ThreadReadOptions;
    using ai::openai::codex::typed::ThreadResumeOptions;
    using ai::openai::codex::typed::ThreadStartOptions;

    Json validTurn(std::string id = "turn-1") {
        return {
            {"id", std::move(id)},
            {"items", Json::array()},
            {"status", "completed"},
            {"startedAt", 10},
            {"completedAt", 11},
            {"durationMs", 1'000},
        };
    }

    Json validThread(std::string id = "thread-1") {
        return {
            {"id", std::move(id)},
            {"cliVersion", "0.144.6"},
            {"createdAt", 1'700'000'000},
            {"cwd", "/tmp/project"},
            {"ephemeral", false},
            {"modelProvider", "openai"},
            {"name", "Typed API work"},
            {"preview", "Implement the typed API"},
            {"sessionId", "session-1"},
            {"source", "appServer"},
            {"status", {{"type", "active"}, {"activeFlags", Json::array({"waitingOnApproval"})}, {"futureStatus", true}}},
            {"turns", Json::array({validTurn()})},
            {"updatedAt", 1'700'000'100},
            {"futureThreadField", Json{{"kept", true}}},
        };
    }

    Json validOperationResult(Json thread = validThread()) {
        return {
            {"approvalPolicy", "on-request"},
            {"approvalsReviewer", "user"},
            {"cwd", "/tmp/project"},
            {"model", "gpt-5.4"},
            {"modelProvider", "openai"},
            {"sandbox", {{"type", "readOnly"}, {"networkAccess", false}}},
            {"thread", std::move(thread)},
            {"futureResultField", Json::array({1, "two"})},
        };
    }

    void testValidThreadAndRawPreservation(tests::support::TestResult& testResult) {
        const Json raw = validThread();
        std::string error = "stale";
        const std::optional<Thread> decoded = decodeThread(raw, error, ModelId{"gpt-5.4"});

        testResult.expectTrue(decoded.has_value(), "current-schema thread is decoded");
        testResult.expectTrue(error.empty(), "successful thread decoding clears a previous error");
        testResult.expectTrue(decoded && decoded->id.value == "thread-1", "thread ID is preserved exactly");
        testResult.expectTrue(decoded && decoded->title == "Typed API work" && decoded->cwd == "/tmp/project",
                              "thread optional title and required cwd are decoded");
        testResult.expectTrue(decoded && decoded->model && decoded->model->value == "gpt-5.4" && decoded->modelProvider == "openai",
                              "thread effective model and provider are preserved");
        testResult.expectTrue(decoded && decoded->preview == "Implement the typed API" && decoded->cliVersion == "0.144.6" &&
                                  decoded->sessionId == "session-1",
                              "thread stable string metadata is decoded");
        testResult.expectTrue(decoded && decoded->createdAt == 1'700'000'000 && decoded->updatedAt == 1'700'000'100 &&
                                  decoded->ephemeral == false,
                              "thread integer and Boolean metadata is decoded");
        testResult.expectTrue(decoded && decoded->status && decoded->status->value == "active" && decoded->status->raw == raw.at("status"),
                              "thread status type and complete raw status are preserved");

        const auto* nestedTurn = decoded && decoded->turns.size() == 1 ? &decoded->turns.front() : nullptr;
        testResult.expectTrue(nestedTurn && nestedTurn->id.value == "turn-1" && nestedTurn->threadId.value == "thread-1",
                              "nested turns are decoded with their parent ThreadId");
        testResult.expectTrue(decoded && decoded->raw == raw, "thread raw JSON preserves unknown fields and the complete original object");
    }

    void testRequiredAndOptionalFields(tests::support::TestResult& testResult) {
        std::string error;

        Json missingId = validThread();
        missingId.erase("id");
        testResult.expectTrue(!decodeThread(missingId, error) && !error.empty(), "thread missing its required ID is rejected");

        Json invalidId = validThread();
        invalidId["id"] = 17;
        testResult.expectTrue(!decodeThread(invalidId, error) && !error.empty(), "thread with a non-string ID is rejected");

        constexpr std::array requiredFields = {
            "cliVersion",
            "createdAt",
            "cwd",
            "ephemeral",
            "modelProvider",
            "preview",
            "sessionId",
            "source",
            "status",
            "turns",
            "updatedAt",
        };
        for (const char* field : requiredFields) {
            Json malformed = validThread();
            malformed.erase(field);
            testResult.expectTrue(!decodeThread(malformed, error) && !error.empty(),
                                  std::string("thread missing current-schema required field '") + field + "' is rejected");
        }

        Json optionalAbsent = validThread();
        optionalAbsent.erase("name");
        const std::optional<Thread> decodedAbsent = decodeThread(optionalAbsent, error);
        testResult.expectTrue(decodedAbsent && !decodedAbsent->title && !decodedAbsent->model,
                              "absent optional title and effective model remain absent");

        Json optionalNull = validThread();
        optionalNull["name"] = nullptr;
        const std::optional<Thread> decodedNull = decodeThread(optionalNull, error);
        testResult.expectTrue(decodedNull && !decodedNull->title, "null optional thread title is accepted as absent");

        Json malformedActiveStatus = validThread();
        malformedActiveStatus["status"].erase("activeFlags");
        testResult.expectTrue(!decodeThread(malformedActiveStatus, error),
                              "known active thread status missing its required activeFlags is rejected");

        Json unknownStatus = validThread();
        unknownStatus["status"] = Json{{"type", "futureStatus"}, {"futureValue", 7}};
        const std::optional<Thread> decodedUnknownStatus = decodeThread(unknownStatus, error);
        testResult.expectTrue(decodedUnknownStatus && decodedUnknownStatus->status &&
                                  decodedUnknownStatus->status->value == "futureStatus" &&
                                  decodedUnknownStatus->status->raw == unknownStatus.at("status"),
                              "unknown future thread status strings and payloads remain nonfatal and raw-preserved");

        Json overflowTimestamp = validThread();
        overflowTimestamp["createdAt"] = std::numeric_limits<std::uint64_t>::max();
        testResult.expectTrue(!decodeThread(overflowTimestamp, error), "thread timestamp outside int64 range is rejected safely");
    }

    void testOperationAndReadWrappers(tests::support::TestResult& testResult) {
        const Json result = validOperationResult();
        std::string error;
        const std::optional<Thread> decoded = decodeThreadOperationResult(result, error);

        testResult.expectTrue(decoded && decoded->id.value == "thread-1", "thread/start or thread/resume result wrapper is decoded");
        testResult.expectTrue(decoded && decoded->model && decoded->model->value == "gpt-5.4",
                              "thread operation wrapper's required model populates the typed thread model");
        testResult.expectTrue(decoded && decoded->raw == result.at("thread"), "typed Thread::raw remains the exact nested thread object");

        constexpr std::array requiredWrapperFields = {
            "approvalPolicy",
            "approvalsReviewer",
            "cwd",
            "model",
            "modelProvider",
            "sandbox",
            "thread",
        };
        for (const char* field : requiredWrapperFields) {
            Json malformed = validOperationResult();
            malformed.erase(field);
            testResult.expectTrue(!decodeThreadOperationResult(malformed, error) && !error.empty(),
                                  std::string("thread operation wrapper missing '") + field + "' is rejected");
        }

        Json malformedThread = validOperationResult();
        malformedThread["thread"]["id"] = Json::array();
        testResult.expectTrue(!decodeThreadOperationResult(malformedThread, error),
                              "thread operation wrapper with a malformed nested thread is rejected");

        Json malformedSandbox = validOperationResult();
        malformedSandbox["sandbox"] = Json::object();
        testResult.expectTrue(!decodeThreadOperationResult(malformedSandbox, error),
                              "thread operation wrapper with no required sandbox discriminator is rejected");

        const Json readResult = {{"thread", validThread()}, {"futureReadField", true}};
        const std::optional<Thread> read = decodeThreadReadResult(readResult, error);
        testResult.expectTrue(read && read->id.value == "thread-1" && read->raw == readResult.at("thread"),
                              "thread/read wrapper returns a typed raw-preserving thread");
        testResult.expectTrue(!decodeThreadReadResult(Json::object(), error) && !error.empty(),
                              "thread/read wrapper missing its required thread is rejected");
    }

    void testListPage(tests::support::TestResult& testResult) {
        Json second = validThread("thread-2");
        second.erase("name");
        const Json rawPage = {
            {"data", Json::array({validThread(), second})},
            {"nextCursor", "next-opaque"},
            {"backwardsCursor", "previous-opaque"},
            {"futurePageField", Json{{"kept", true}}},
        };

        std::string error;
        const auto page = decodeThreadListResult(rawPage, error);
        testResult.expectTrue(page && page->data.size() == 2, "thread/list decodes every current-schema thread entry");
        testResult.expectTrue(page && page->nextCursor == "next-opaque" && page->backwardsCursor == "previous-opaque",
                              "thread/list preserves forward and backwards opaque cursors");
        testResult.expectTrue(page && page->raw == rawPage, "thread/list preserves the complete raw page and unknown fields");

        const Json emptyRaw = {{"data", Json::array()}, {"nextCursor", nullptr}, {"backwardsCursor", nullptr}};
        const auto empty = decodeThreadListResult(emptyRaw, error);
        testResult.expectTrue(empty && empty->data.empty() && !empty->nextCursor && !empty->backwardsCursor,
                              "empty thread/list page and null cursors decode successfully");

        Json malformedEntry = validThread();
        malformedEntry.erase("id");
        const Json malformedPage = {{"data", Json::array({validThread(), malformedEntry})}};
        testResult.expectTrue(!decodeThreadListResult(malformedPage, error) && !error.empty(),
                              "one malformed thread/list entry fails the typed page instead of being dropped");
        testResult.expectTrue(!decodeThreadListResult(Json::object(), error), "thread/list page missing required data is rejected");
        testResult.expectTrue(!decodeThreadListResult(Json{{"data", Json::array()}, {"nextCursor", 9}}, error),
                              "thread/list rejects a malformed optional cursor");
    }

    void testOptionEncoding(tests::support::TestResult& testResult) {
        std::string error = "stale";
        ThreadStartOptions start;
        start.cwd = "/tmp/start";
        start.model = ModelId{"gpt-5.4"};
        start.modelProvider = "openai";
        start.approvalPolicy = ApprovalPolicy{"on-request"};
        start.sandboxMode = SandboxMode{"workspace-write"};
        start.ephemeral = true;
        const Json expectedStart = {
            {"cwd", "/tmp/start"},
            {"model", "gpt-5.4"},
            {"modelProvider", "openai"},
            {"approvalPolicy", "on-request"},
            {"sandbox", "workspace-write"},
            {"ephemeral", true},
        };
        const std::optional<Json> encodedStart = encodeThreadStartParams(start, error);
        testResult.expectTrue(encodedStart && *encodedStart == expectedStart && error.empty(),
                              "thread/start options use exact current-schema field names and extensible string values");
        const std::optional<Json> minimumStart = encodeThreadStartParams({}, error);
        testResult.expectTrue(minimumStart && *minimumStart == Json::object(), "thread/start omits every unsupported or absent option");

        ThreadResumeOptions resume;
        resume.cwd = "/tmp/resume";
        resume.model = ModelId{"future-model"};
        resume.modelProvider = "future-provider";
        resume.approvalPolicy = ApprovalPolicy{"future-policy"};
        resume.sandboxMode = SandboxMode{"future-sandbox"};
        const Json expectedResume = {
            {"threadId", "thread-resume"},
            {"cwd", "/tmp/resume"},
            {"model", "future-model"},
            {"modelProvider", "future-provider"},
            {"approvalPolicy", "future-policy"},
            {"sandbox", "future-sandbox"},
        };
        const std::optional<Json> encodedResume = encodeThreadResumeParams(ThreadId{"thread-resume"}, resume, error);
        testResult.expectTrue(encodedResume && *encodedResume == expectedResume,
                              "thread/resume encodes the strong ID and preserves extensible option values");

        ThreadListOptions list;
        list.cursor = "cursor";
        list.limit = 25;
        list.archived = true;
        list.searchTerm = "typed";
        const std::optional<Json> encodedList = encodeThreadListParams(list, error);
        testResult.expectTrue(encodedList &&
                                  *encodedList == Json{{"cursor", "cursor"}, {"limit", 25}, {"archived", true}, {"searchTerm", "typed"}},
                              "thread/list encodes supported pagination and filter options");

        ThreadReadOptions read;
        read.includeTurns = true;
        const std::optional<Json> encodedRead = encodeThreadReadParams(ThreadId{"thread-read"}, read, error);
        testResult.expectTrue(encodedRead && *encodedRead == Json{{"threadId", "thread-read"}, {"includeTurns", true}},
                              "thread/read encodes its strong ID and includeTurns option");
    }
} // namespace

int main() {
    tests::support::TestResult testResult;

    testValidThreadAndRawPreservation(testResult);
    testRequiredAndOptionalFields(testResult);
    testOperationAndReadWrappers(testResult);
    testListPage(testResult);
    testOptionEncoding(testResult);

    return testResult.processResult();
}
