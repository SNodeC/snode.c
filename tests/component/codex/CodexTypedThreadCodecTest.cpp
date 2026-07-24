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
#include <type_traits>
#include <variant>
#include <vector>

namespace {
    using ai::openai::codex::Json;
    using ai::openai::codex::detail::decodeThread;
    using ai::openai::codex::detail::decodeThreadListResult;
    using ai::openai::codex::detail::decodeThreadOperationResult;
    using ai::openai::codex::detail::decodeThreadReadResult;
    using ai::openai::codex::detail::decodeThreadStartResponse;
    using ai::openai::codex::detail::decodeSessionSource;
    using ai::openai::codex::detail::decodeSubAgentSource;
    using ai::openai::codex::detail::decodeThreadStatus;
    using ai::openai::codex::detail::decodeUnitResult;
    using ai::openai::codex::detail::encodeThreadListParams;
    using ai::openai::codex::detail::encodeThreadReadParams;
    using ai::openai::codex::detail::encodeThreadResumeParams;
    using ai::openai::codex::detail::encodeThreadStartParams;
    using ai::openai::codex::typed::ApprovalPolicy;
    using ai::openai::codex::typed::ActiveThreadStatus;
    using ai::openai::codex::typed::AskForApproval;
    using ai::openai::codex::typed::ClientUserMessageId;
    using ai::openai::codex::typed::CustomSessionSource;
    using ai::openai::codex::typed::DecodeIssueKind;
    using ai::openai::codex::typed::DecodeIssueSeverity;
    using ai::openai::codex::typed::IdleThreadStatus;
    using ai::openai::codex::typed::LegacyAppPathString;
    using ai::openai::codex::typed::ModelId;
    using ai::openai::codex::typed::OtherSubAgentSource;
    using ai::openai::codex::typed::SandboxMode;
    using ai::openai::codex::typed::SessionId;
    using ai::openai::codex::typed::SessionSourceKind;
    using ai::openai::codex::typed::SystemErrorThreadStatus;
    using ai::openai::codex::typed::SubAgentSessionSource;
    using ai::openai::codex::typed::SubAgentSourceKind;
    using ai::openai::codex::typed::Thread;
    using ai::openai::codex::typed::ThreadForkResponse;
    using ai::openai::codex::typed::ThreadId;
    using ai::openai::codex::typed::ThreadLoadedListResponse;
    using ai::openai::codex::typed::ThreadListOptions;
    using ai::openai::codex::typed::ThreadReadOptions;
    using ai::openai::codex::typed::ThreadResumeParams;
    using ai::openai::codex::typed::ThreadResumeOptions;
    using ai::openai::codex::typed::ThreadResumeResponse;
    using ai::openai::codex::typed::ThreadStartOptions;
    using ai::openai::codex::typed::ThreadStartResponse;
    using ai::openai::codex::typed::ThreadSpawnSubAgentDetails;
    using ai::openai::codex::typed::ThreadSpawnSubAgentSource;
    using ai::openai::codex::typed::NotLoadedThreadStatus;
    using ai::openai::codex::typed::UnknownSessionSourceAlternative;
    using ai::openai::codex::typed::UnknownSubAgentSource;
    using ai::openai::codex::typed::UnknownThreadStatus;
    using ai::openai::codex::typed::Turn;
    using ai::openai::codex::typed::TurnItemsView;
    using ai::openai::codex::typed::TurnStartParams;
    using ai::openai::codex::typed::TurnSteerParams;
    using ai::openai::codex::typed::UserMessageThreadItem;
    using ai::openai::codex::typed::threadStatusDiscriminator;
    using ai::openai::codex::typed::threadStatusRaw;

    static_assert(std::is_same_v<decltype(ThreadSpawnSubAgentSource::threadSpawn), ThreadSpawnSubAgentDetails>,
                  "the reviewed SubAgentSource/thread_spawn closure mapping must name "
                  "the handwritten nested aggregate");
    static_assert(std::is_same_v<decltype(Turn::itemsView), TurnItemsView>, "Turn.itemsView must materialize its reviewed schema default");
    static_assert(std::is_same_v<decltype(Thread::sessionId), SessionId>,
                  "Thread.sessionId must remain a distinct App Server conversation-tree identifier");
    using InstructionSources = std::vector<LegacyAppPathString>;
    static_assert(std::is_same_v<decltype(ThreadLoadedListResponse::data), std::vector<ThreadId>>,
                  "ThreadLoadedListResponse.data must preserve its semantic ThreadId "
                  "element type");
    using OptionalClientUserMessageId = ai::openai::codex::typed::OptionalNullable<ClientUserMessageId>;
    static_assert(std::is_same_v<decltype(TurnStartParams::clientUserMessageId), OptionalClientUserMessageId> &&
                      std::is_same_v<decltype(TurnSteerParams::clientUserMessageId), OptionalClientUserMessageId> &&
                      std::is_same_v<decltype(UserMessageThreadItem::clientId), OptionalClientUserMessageId>,
                  "turn request clientUserMessageId and user-message clientId "
                  "must share one semantic echo identity");
    static_assert(std::is_same_v<decltype(ThreadForkResponse::instructionSources), InstructionSources> &&
                      std::is_same_v<decltype(ThreadResumeResponse::instructionSources), InstructionSources> &&
                      std::is_same_v<decltype(ThreadStartResponse::instructionSources), InstructionSources>,
                  "thread response instructionSources fields must materialize the "
                  "reviewed empty-array schema default");

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
            {"gitInfo",
             {{"branch", "main"},
              {"originUrl", "https://example.invalid/repo.git"},
              {"sha", "0123456789abcdef"},
              {"futureGitField", {{"kept", true}}}}},
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
        testResult.expectTrue(decoded && decoded->name.hasValue() && *decoded->name == "Typed API work" &&
                                  decoded->title == "Typed API work" && decoded->cwd.value == "/tmp/project",
                              "canonical thread name and required strong cwd decode while title mirrors valued name");
        testResult.expectTrue(decoded && decoded->model && decoded->model->value == "gpt-5.4" && decoded->modelProvider == "openai",
                              "thread effective model and provider are preserved");
        testResult.expectTrue(decoded && decoded->preview == "Implement the typed API" && decoded->cliVersion == "0.144.6" &&
                                  decoded->sessionId.value == "session-1",
                              "thread stable string metadata is decoded");
        testResult.expectTrue(decoded && decoded->createdAt == 1'700'000'000 && decoded->updatedAt == 1'700'000'100 &&
                                  decoded->ephemeral == false,
                              "thread integer and Boolean metadata is decoded");
        testResult.expectTrue(
            decoded && decoded->gitInfo.hasValue() &&
                decoded->gitInfo->branch.hasValue() &&
                *decoded->gitInfo->branch == "main" &&
                decoded->gitInfo->originUrl.hasValue() &&
                decoded->gitInfo->sha.hasValue() &&
                decoded->gitInfo->raw == raw.at("gitInfo") &&
                decoded->gitInfo->raw.at("futureGitField").at("kept") == true,
            "incoming GitInfo retains its narrow exact object and unknown fields");
        const auto* activeStatus = decoded ? std::get_if<ActiveThreadStatus>(&decoded->status) : nullptr;
        testResult.expectTrue(activeStatus && activeStatus->activeFlags.size() == 1 &&
                                  activeStatus->activeFlags.front().value == "waitingOnApproval" &&
                                  threadStatusDiscriminator(decoded->status) == "active" &&
                                  threadStatusRaw(decoded->status) == raw.at("status"),
                              "thread status selects the active variant and preserves fields and complete raw status");

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
        testResult.expectTrue(decodedAbsent && decodedAbsent->name.isOmitted() && !decodedAbsent->title && !decodedAbsent->model,
                              "omitted canonical name remains distinct and compatibility title/model remain absent");

        Json optionalNull = validThread();
        optionalNull["name"] = nullptr;
        const std::optional<Thread> decodedNull = decodeThread(optionalNull, error);
        testResult.expectTrue(decodedNull && decodedNull->name.isNull() && !decodedNull->title,
                              "explicit null canonical name remains distinct while compatibility title is absent");

        Json malformedActiveStatus = validThread();
        malformedActiveStatus["status"].erase("activeFlags");
        testResult.expectTrue(!decodeThread(malformedActiveStatus, error),
                              "known active thread status missing its required activeFlags is rejected");

        Json unknownStatus = validThread();
        unknownStatus["status"] = Json{{"type", "futureStatus"}, {"futureValue", 7}};
        const std::optional<Thread> decodedUnknownStatus = decodeThread(unknownStatus, error);
        const auto* futureStatus =
            decodedUnknownStatus ? std::get_if<UnknownThreadStatus>(&decodedUnknownStatus->status) : nullptr;
        testResult.expectTrue(futureStatus && futureStatus->type == "futureStatus" &&
                                  futureStatus->raw == unknownStatus.at("status") && futureStatus->diagnostic &&
                                  futureStatus->diagnostic->kind == DecodeIssueKind::UnknownDiscriminator &&
                                  futureStatus->diagnostic->severity == DecodeIssueSeverity::ForwardCompatibility &&
                                  futureStatus->diagnostic->surface == "ThreadStatus" &&
                                  futureStatus->diagnostic->fieldPath == "$.type",
                              "future ThreadStatus selects its explicit unknown alternative with exact diagnostic identity");

        Json futureActiveFlag = validThread();
        futureActiveFlag["status"]["activeFlags"].push_back("futureFlag");
        const std::optional<Thread> decodedFutureActiveFlag = decodeThread(futureActiveFlag, error);
        const auto* activeWithFuture =
            decodedFutureActiveFlag ? std::get_if<ActiveThreadStatus>(&decodedFutureActiveFlag->status) : nullptr;
        testResult.expectTrue(activeWithFuture && activeWithFuture->activeFlags.size() == 2 &&
                                  activeWithFuture->diagnostics.size() == 1 &&
                                  activeWithFuture->diagnostics.front().kind == DecodeIssueKind::UnknownEnumValue &&
                                  activeWithFuture->diagnostics.front().severity == DecodeIssueSeverity::ForwardCompatibility &&
                                  activeWithFuture->diagnostics.front().surface == "ThreadActiveFlag" &&
                                  activeWithFuture->diagnostics.front().fieldPath == "$.activeFlags[1]",
                              "future active flag stays in the known active variant with an exact open-enum diagnostic");

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

        Json futureNested = validOperationResult();
        futureNested["thread"]["status"] = {
            {"type", "futureThreadStatus"},
            {"futureStatusField", Json{{"retained", true}}},
        };
        futureNested["thread"]["turns"][0]["items"].push_back(
            {{"type", "futureThreadItem"},
             {"id", "future-item"},
             {"futureItemField", Json{{"retained", true}}}});
        const auto acceptedFutureNested =
            decodeThreadStartResponse(futureNested, error);
        const auto* futureStatus =
            acceptedFutureNested
                ? std::get_if<UnknownThreadStatus>(
                      &acceptedFutureNested->thread.status)
                : nullptr;
        const auto* futureItem =
            acceptedFutureNested &&
                    acceptedFutureNested->thread.turns.size() == 1 &&
                    acceptedFutureNested->thread.turns.front().items.size() == 1
                ? std::get_if<ai::openai::codex::typed::UnknownItem>(
                      &acceptedFutureNested->thread.turns.front().items.front())
                : nullptr;
        testResult.expectTrue(
            acceptedFutureNested && error.empty() && futureStatus &&
                futureStatus->raw == futureNested["thread"]["status"] &&
                futureStatus->diagnostic &&
                futureStatus->diagnostic->kind ==
                    DecodeIssueKind::UnknownDiscriminator &&
                futureStatus->diagnostic->severity ==
                    DecodeIssueSeverity::ForwardCompatibility &&
                futureItem &&
                futureItem->raw ==
                    futureNested["thread"]["turns"][0]["items"][0] &&
                futureItem->diagnostic &&
                futureItem->diagnostic->kind ==
                    DecodeIssueKind::UnknownDiscriminator &&
                futureItem->diagnostic->severity ==
                    DecodeIssueSeverity::ForwardCompatibility,
            "strict result wrappers accept raw-retaining future ThreadStatus "
            "and ThreadItem alternatives while rejecting only malformed-known "
            "payload diagnostics");

        const Json readResult = {{"thread", validThread()}, {"futureReadField", true}};
        const std::optional<Thread> read = decodeThreadReadResult(readResult, error);
        testResult.expectTrue(read && read->id.value == "thread-1" && read->raw == readResult.at("thread"),
                              "thread/read wrapper returns a typed raw-preserving thread");
        testResult.expectTrue(!decodeThreadReadResult(Json::object(), error) && !error.empty(),
                              "thread/read wrapper missing its required thread is rejected");

        const auto omitted = decodeThreadStartResponse(result, error);
        testResult.expectTrue(omitted && omitted->serviceTier.isOmitted() &&
                                  omitted->reasoningEffort.isOmitted() && omitted->instructionSources.empty() &&
                                  !omitted->raw.contains("instructionSources") && omitted->raw == result,
                              "canonical launch response applies the empty instructionSources default while raw retains omission");

        Json nullable = result;
        nullable["serviceTier"] = nullptr;
        nullable["reasoningEffort"] = nullptr;
        const auto explicitNull = decodeThreadStartResponse(nullable, error);
        testResult.expectTrue(explicitNull && explicitNull->serviceTier.isNull() &&
                                  explicitNull->reasoningEffort.isNull(),
                              "canonical launch response distinguishes explicit null from omission");

        Json futureEnums = result;
        futureEnums["serviceTier"] = "priority";
        futureEnums["reasoningEffort"] = "synthetic-b43c-reasoning";
        futureEnums["approvalsReviewer"] = "future-reviewer";
        futureEnums["instructionSources"] = Json::array({"/tmp/AGENTS.md"});
        const auto decodedFutureEnums = decodeThreadStartResponse(futureEnums, error);
        testResult.expectTrue(decodedFutureEnums && decodedFutureEnums->serviceTier.hasValue() &&
                                  *decodedFutureEnums->serviceTier == "priority" &&
                                  decodedFutureEnums->reasoningEffort.hasValue() &&
                                  decodedFutureEnums->reasoningEffort->value == "synthetic-b43c-reasoning" &&
                                  decodedFutureEnums->instructionSources.size() == 1 &&
                                  decodedFutureEnums->instructionSources.front().value == "/tmp/AGENTS.md" &&
                                  decodedFutureEnums->diagnostics.size() == 2 &&
                                  decodedFutureEnums->diagnostics[0].kind == DecodeIssueKind::UnknownEnumValue &&
                                  decodedFutureEnums->diagnostics[0].surface == "ApprovalsReviewer" &&
                                  decodedFutureEnums->diagnostics[0].fieldPath == "$.approvalsReviewer" &&
                                  decodedFutureEnums->diagnostics[1].kind == DecodeIssueKind::UnknownEnumValue &&
                                  decodedFutureEnums->diagnostics[1].surface == "ReasoningEffort" &&
                                  decodedFutureEnums->diagnostics[1].fieldPath == "$.reasoningEffort",
                              "launch response retains valued fields and exact future open-enum diagnostics");
    }

    void testConversationUnionFoundation(tests::support::TestResult& testResult) {
        const auto notLoaded = decodeThreadStatus(Json{{"type", "notLoaded"}});
        const auto idle = decodeThreadStatus(Json{{"type", "idle"}});
        const auto systemError = decodeThreadStatus(Json{{"type", "systemError"}});
        testResult.expectTrue(notLoaded.value && std::holds_alternative<NotLoadedThreadStatus>(*notLoaded.value) &&
                                  !notLoaded.diagnostic && idle.value &&
                                  std::holds_alternative<IdleThreadStatus>(*idle.value) && !idle.diagnostic &&
                                  systemError.value &&
                                  std::holds_alternative<SystemErrorThreadStatus>(*systemError.value) &&
                                  !systemError.diagnostic,
                              "all three fieldless ThreadStatus alternatives select distinct public types");

        const auto missingStatusType = decodeThreadStatus(Json::object());
        const auto primitiveStatus = decodeThreadStatus(Json("idle"));
        testResult.expectTrue(!missingStatusType.value && missingStatusType.diagnostic &&
                                  missingStatusType.diagnostic->kind == DecodeIssueKind::MalformedKnownPayload &&
                                  missingStatusType.diagnostic->severity == DecodeIssueSeverity::ProtocolWarning &&
                                  missingStatusType.diagnostic->surface == "ThreadStatus" &&
                                  missingStatusType.diagnostic->fieldPath == "$.type" &&
                                  !primitiveStatus.value && primitiveStatus.diagnostic &&
                                  primitiveStatus.diagnostic->kind == DecodeIssueKind::MalformedKnownPayload &&
                                  primitiveStatus.diagnostic->severity == DecodeIssueSeverity::ProtocolWarning &&
                                  primitiveStatus.diagnostic->surface == "ThreadStatus" &&
                                  primitiveStatus.diagnostic->fieldPath == "$",
                              "missing ThreadStatus discriminator and wrong outer primitive report exact malformed-known diagnostics");

        const auto wrongActiveFlag =
            decodeThreadStatus(Json{{"type", "active"}, {"activeFlags", Json::array({"waitingOnApproval", 7})}});
        testResult.expectTrue(!wrongActiveFlag.value && wrongActiveFlag.diagnostic &&
                                  wrongActiveFlag.diagnostic->kind == DecodeIssueKind::MalformedKnownPayload &&
                                  wrongActiveFlag.diagnostic->severity == DecodeIssueSeverity::ProtocolWarning &&
                                  wrongActiveFlag.diagnostic->surface == "ThreadStatus" &&
                                  wrongActiveFlag.diagnostic->fieldPath == "$.activeFlags[1]",
                              "wrong nested active-flag type is malformed-known at the exact field path");

        constexpr std::array sessionScalars = {"cli", "vscode", "exec", "appServer", "unknown"};
        for (const char* discriminator : sessionScalars) {
            const auto decoded = decodeSessionSource(Json(discriminator));
            const auto* scalar = decoded.value ? std::get_if<SessionSourceKind>(&*decoded.value) : nullptr;
            testResult.expectTrue(scalar && scalar->value == discriminator && scalar->raw == discriminator &&
                                      !decoded.diagnostic,
                                  std::string("SessionSource scalar '") + discriminator +
                                      "' selects its registry-backed typed value");
        }

        const Json futureSessionScalar = "future-session-secret";
        const auto futureScalarSession = decodeSessionSource(futureSessionScalar);
        const auto* futureScalarSessionValue =
            futureScalarSession.value ? std::get_if<UnknownSessionSourceAlternative>(&*futureScalarSession.value) : nullptr;
        testResult.expectTrue(futureScalarSessionValue &&
                                  futureScalarSessionValue->discriminator == "future-session-secret" &&
                                  futureScalarSessionValue->raw == futureSessionScalar &&
                                  futureScalarSessionValue->diagnostic &&
                                  futureScalarSession.diagnostic &&
                                  futureScalarSession.diagnostic->kind == DecodeIssueKind::UnknownDiscriminator &&
                                  futureScalarSession.diagnostic->severity == DecodeIssueSeverity::ForwardCompatibility &&
                                  futureScalarSession.diagnostic->surface == "SessionSource" &&
                                  futureScalarSession.diagnostic->fieldPath == "$" &&
                                  futureScalarSession.diagnostic->message.find("future-session-secret") == std::string::npos,
                              "future scalar SessionSource selects its explicit raw-retaining unknown alternative");

        const Json customRaw = {{"custom", "integration-test"}};
        const auto custom = decodeSessionSource(customRaw);
        const auto* customValue = custom.value ? std::get_if<CustomSessionSource>(&*custom.value) : nullptr;
        testResult.expectTrue(customValue && customValue->custom == "integration-test" && customValue->raw == customRaw,
                              "custom SessionSource selects its typed externally-tagged alternative");

        const Json threadSpawn = {
            {"thread_spawn",
             {{"parent_thread_id", "parent"},
              {"depth", 2},
              {"agent_path", "planner/reviewer"},
              {"agent_nickname", nullptr}}},
        };
        const auto spawned = decodeSubAgentSource(threadSpawn);
        const auto* spawn = spawned.value ? std::get_if<ThreadSpawnSubAgentSource>(&*spawned.value) : nullptr;
        testResult.expectTrue(spawn && spawn->threadSpawn.parentThreadId.value == "parent" &&
                                  spawn->threadSpawn.depth == 2 && spawn->threadSpawn.agentPath.hasValue() &&
                                  spawn->threadSpawn.agentPath->value == "planner/reviewer" &&
                                  spawn->threadSpawn.agentNickname.isNull() && spawn->threadSpawn.agentRole.isOmitted() &&
                                  spawn->raw == threadSpawn,
                              "thread_spawn SubAgentSource preserves required fields, tri-state optionals, and raw input");

        for (const char* discriminator : {"review", "compact", "memory_consolidation"}) {
            const auto decoded = decodeSubAgentSource(Json(discriminator));
            const auto* scalar = decoded.value ? std::get_if<SubAgentSourceKind>(&*decoded.value) : nullptr;
            testResult.expectTrue(scalar && scalar->value == discriminator && !decoded.diagnostic,
                                  std::string("SubAgentSource scalar '") + discriminator +
                                      "' selects its registry-backed typed value");
        }

        const Json futureSubAgentScalar = "future-sub-agent-secret";
        const auto futureScalarSubAgent = decodeSubAgentSource(futureSubAgentScalar);
        const auto* futureScalarSubAgentValue =
            futureScalarSubAgent.value ? std::get_if<UnknownSubAgentSource>(&*futureScalarSubAgent.value) : nullptr;
        testResult.expectTrue(futureScalarSubAgentValue &&
                                  futureScalarSubAgentValue->discriminator == "future-sub-agent-secret" &&
                                  futureScalarSubAgentValue->raw == futureSubAgentScalar &&
                                  futureScalarSubAgentValue->diagnostic &&
                                  futureScalarSubAgent.diagnostic &&
                                  futureScalarSubAgent.diagnostic->kind == DecodeIssueKind::UnknownDiscriminator &&
                                  futureScalarSubAgent.diagnostic->severity == DecodeIssueSeverity::ForwardCompatibility &&
                                  futureScalarSubAgent.diagnostic->surface == "SubAgentSource" &&
                                  futureScalarSubAgent.diagnostic->fieldPath == "$" &&
                                  futureScalarSubAgent.diagnostic->message.find("future-sub-agent-secret") == std::string::npos,
                              "future scalar SubAgentSource selects its explicit raw-retaining unknown alternative");

        const Json otherRaw = {{"other", "migration"}};
        const auto other = decodeSubAgentSource(otherRaw);
        const auto* otherValue = other.value ? std::get_if<OtherSubAgentSource>(&*other.value) : nullptr;
        testResult.expectTrue(otherValue && otherValue->other == "migration" && otherValue->raw == otherRaw,
                              "other SubAgentSource selects its typed externally-tagged alternative");

        const Json subAgentRaw = {{"subAgent", "review"}};
        const auto subAgent = decodeSessionSource(subAgentRaw);
        const auto* subAgentValue = subAgent.value ? std::get_if<SubAgentSessionSource>(&*subAgent.value) : nullptr;
        testResult.expectTrue(subAgentValue && subAgentValue->raw == subAgentRaw,
                              "subAgent SessionSource composes the nested production decoder");

        const Json futureNestedSubAgentRaw = {{"subAgent", "future-sub-agent"}};
        const auto futureNestedSubAgent =
            decodeSessionSource(futureNestedSubAgentRaw);
        const auto* futureNestedOuter =
            futureNestedSubAgent.value
                ? std::get_if<SubAgentSessionSource>(
                      &*futureNestedSubAgent.value)
                : nullptr;
        const auto* futureNestedInner =
            futureNestedOuter
                ? std::get_if<UnknownSubAgentSource>(
                      &futureNestedOuter->subAgent)
                : nullptr;
        testResult.expectTrue(
            futureNestedInner &&
                futureNestedInner->raw == Json("future-sub-agent") &&
                futureNestedSubAgent.diagnostic &&
                futureNestedSubAgent.diagnostic->kind ==
                    DecodeIssueKind::UnknownDiscriminator &&
                futureNestedSubAgent.diagnostic->severity ==
                    DecodeIssueSeverity::ForwardCompatibility &&
                futureNestedSubAgent.diagnostic->surface ==
                    "SubAgentSource" &&
                futureNestedSubAgent.diagnostic->fieldPath == "$.subAgent" &&
                futureNestedOuter->diagnostics.size() == 1 &&
                futureNestedOuter->diagnostics.front().fieldPath ==
                    "$.subAgent",
            "known SessionSource subAgent retains an explicit future inner "
            "alternative and prefixes its diagnostic path");

        const Json futureSessionRaw = {{"futureSessionSource", {{"secret", "not-diagnosed"}}}};
        const auto futureSession = decodeSessionSource(futureSessionRaw);
        const auto* unknownSession =
            futureSession.value ? std::get_if<UnknownSessionSourceAlternative>(&*futureSession.value) : nullptr;
        testResult.expectTrue(unknownSession && unknownSession->raw == futureSessionRaw &&
                                  futureSession.diagnostic &&
                                  futureSession.diagnostic->kind == DecodeIssueKind::UnknownDiscriminator &&
                                  futureSession.diagnostic->severity == DecodeIssueSeverity::ForwardCompatibility &&
                                  futureSession.diagnostic->surface == "SessionSource" &&
                                  futureSession.diagnostic->fieldPath == "$" &&
                                  futureSession.diagnostic->message.find("not-diagnosed") == std::string::npos,
                              "future SessionSource uses an explicit raw-retaining unknown alternative without payload leakage");

        const Json futureSubAgentRaw = {{"future_sub_agent", {{"secret", "also-not-diagnosed"}}}};
        const auto futureSubAgent = decodeSubAgentSource(futureSubAgentRaw);
        const auto* unknownSubAgent =
            futureSubAgent.value ? std::get_if<UnknownSubAgentSource>(&*futureSubAgent.value) : nullptr;
        testResult.expectTrue(unknownSubAgent && unknownSubAgent->raw == futureSubAgentRaw &&
                                  futureSubAgent.diagnostic &&
                                  futureSubAgent.diagnostic->kind == DecodeIssueKind::UnknownDiscriminator &&
                                  futureSubAgent.diagnostic->severity == DecodeIssueSeverity::ForwardCompatibility &&
                                  futureSubAgent.diagnostic->surface == "SubAgentSource" &&
                                  futureSubAgent.diagnostic->fieldPath == "$" &&
                                  futureSubAgent.diagnostic->message.find("also-not-diagnosed") == std::string::npos,
                              "future SubAgentSource uses its distinct raw-retaining unknown alternative");

        const auto malformedKnown = decodeSessionSource(Json{{"custom", 7}});
        testResult.expectTrue(!malformedKnown.value && malformedKnown.diagnostic &&
                                  malformedKnown.diagnostic->kind == DecodeIssueKind::MalformedKnownPayload &&
                                  malformedKnown.diagnostic->severity == DecodeIssueSeverity::ProtocolWarning &&
                                  malformedKnown.diagnostic->surface == "SessionSource" &&
                                  malformedKnown.diagnostic->fieldPath == "$.custom",
                              "malformed known SessionSource is a protocol warning, not forward compatibility");

        const auto conflictingSession =
            decodeSessionSource(Json{{"custom", "one"}, {"subAgent", "review"}});
        const auto nullSession = decodeSessionSource(Json(nullptr));
        const auto arraySubAgent = decodeSubAgentSource(Json::array({"review"}));
        testResult.expectTrue(!conflictingSession.value && conflictingSession.diagnostic &&
                                  conflictingSession.diagnostic->kind == DecodeIssueKind::MalformedKnownPayload &&
                                  conflictingSession.diagnostic->severity == DecodeIssueSeverity::ProtocolWarning &&
                                  conflictingSession.diagnostic->surface == "SessionSource" &&
                                  conflictingSession.diagnostic->fieldPath == "$" &&
                                  !nullSession.value && nullSession.diagnostic &&
                                  nullSession.diagnostic->kind == DecodeIssueKind::MalformedKnownPayload &&
                                  nullSession.diagnostic->severity == DecodeIssueSeverity::ProtocolWarning &&
                                  nullSession.diagnostic->surface == "SessionSource" &&
                                  nullSession.diagnostic->fieldPath == "$" &&
                                  !arraySubAgent.value && arraySubAgent.diagnostic &&
                                  arraySubAgent.diagnostic->kind == DecodeIssueKind::MalformedKnownPayload &&
                                  arraySubAgent.diagnostic->severity == DecodeIssueSeverity::ProtocolWarning &&
                                  arraySubAgent.diagnostic->surface == "SubAgentSource" &&
                                  arraySubAgent.diagnostic->fieldPath == "$",
                              "conflicting SessionSource tags and wrong outer union shapes report exact malformed-known diagnostics");

        const auto missingSpawnParent =
            decodeSubAgentSource(Json{{"thread_spawn", Json{{"depth", 1}}}});
        testResult.expectTrue(!missingSpawnParent.value && missingSpawnParent.diagnostic &&
                                  missingSpawnParent.diagnostic->kind == DecodeIssueKind::MalformedKnownPayload &&
                                  missingSpawnParent.diagnostic->severity == DecodeIssueSeverity::ProtocolWarning &&
                                  missingSpawnParent.diagnostic->surface == "SubAgentSource" &&
                                  missingSpawnParent.diagnostic->fieldPath == "$.thread_spawn.parent_thread_id",
                              "thread_spawn missing parent_thread_id reports its exact malformed-known field path");

        const Json malformedNestedSession = {
            {"subAgent",
             {{"thread_spawn",
               {{"parent_thread_id", "parent-secret"},
                {"depth", 1},
                {"agent_path", false}}}}},
        };
        const auto prefixedNestedSession =
            decodeSessionSource(malformedNestedSession);
        testResult.expectTrue(
            !prefixedNestedSession.value &&
                prefixedNestedSession.diagnostic &&
                prefixedNestedSession.diagnostic->kind ==
                    DecodeIssueKind::MalformedKnownPayload &&
                prefixedNestedSession.diagnostic->severity ==
                    DecodeIssueSeverity::ProtocolWarning &&
                prefixedNestedSession.diagnostic->surface == "SubAgentSource" &&
                prefixedNestedSession.diagnostic->fieldPath ==
                    "$.subAgent.thread_spawn.agent_path" &&
                prefixedNestedSession.diagnostic->message.find(
                    "parent-secret") == std::string::npos,
            "SessionSource prefixes and preserves the exact nested "
            "SubAgentSource failure path without exposing payload values");

        const auto conflicting =
            decodeSubAgentSource(Json{{"other", "one"}, {"thread_spawn", Json::object()}});
        testResult.expectTrue(!conflicting.value && conflicting.diagnostic &&
                                  conflicting.diagnostic->kind == DecodeIssueKind::MalformedKnownPayload &&
                                  conflicting.diagnostic->severity == DecodeIssueSeverity::ProtocolWarning &&
                                  conflicting.diagnostic->surface == "SubAgentSource" &&
                                  conflicting.diagnostic->fieldPath == "$",
                              "multiple externally-tagged SubAgentSource discriminators report the exact malformed-known diagnostic");
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
        testResult.expectTrue(page && page->nextCursor.hasValue() && *page->nextCursor == "next-opaque" &&
                                  page->backwardsCursor.hasValue() && *page->backwardsCursor == "previous-opaque",
                              "thread/list preserves forward and backwards opaque cursors");
        testResult.expectTrue(page && page->raw == rawPage, "thread/list preserves the complete raw page and unknown fields");

        const Json emptyRaw = {{"data", Json::array()}, {"nextCursor", nullptr}, {"backwardsCursor", nullptr}};
        const auto empty = decodeThreadListResult(emptyRaw, error);
        testResult.expectTrue(empty && empty->data.empty() && empty->nextCursor.isNull() && empty->backwardsCursor.isNull(),
                              "empty thread/list page preserves explicit-null cursor state");

        const auto omitted = decodeThreadListResult(Json{{"data", Json::array()}}, error);
        testResult.expectTrue(omitted && omitted->nextCursor.isOmitted() && omitted->backwardsCursor.isOmitted(),
                              "thread/list distinguishes omitted cursors from explicit null");

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
        const std::optional<Json> minimumStart = encodeThreadStartParams(ThreadStartOptions{}, error);
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
        testResult.expectTrue(encodedResume && *encodedResume == expectedResume && error.empty(),
                              "deprecated thread/resume preserves future non-empty approval and sandbox values on the exact wire");

        ThreadResumeParams canonicalResume;
        canonicalResume.threadId = ThreadId{"thread-canonical"};
        canonicalResume.approvalPolicy = AskForApproval{ApprovalPolicy{"future-canonical-policy"}};
        const auto encodedCanonicalResume = encodeThreadResumeParams(canonicalResume, error);
        testResult.expectTrue(encodedCanonicalResume &&
                                  *encodedCanonicalResume ==
                                      Json{{"threadId", "thread-canonical"}, {"approvalPolicy", "future-canonical-policy"}} &&
                                  error.empty(),
                              "canonical thread/resume preserves the same open non-empty approval scalar");

        resume.approvalPolicy = ApprovalPolicy{""};
        testResult.expectTrue(!encodeThreadResumeParams(ThreadId{"thread-resume"}, resume, error) && !error.empty(),
                              "thread/resume rejects an empty approval scalar prohibited by the open value contract");

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

    void testExactUnitContract(tests::support::TestResult& testResult) {
        std::string error = "stale";
        testResult.expectTrue(decodeUnitResult(Json::object(), error).has_value() && error.empty(),
                              "Unit accepts the pinned exact empty-object successful result");
        testResult.expectTrue(!decodeUnitResult(Json{{"unexpected", true}}, error) &&
                                  error == "Unit successful result must be the exact empty object",
                              "Unit rejects a schema-property mutation with the exact stable diagnostic");
        testResult.expectTrue(!decodeUnitResult(nullptr, error) &&
                                  error == "Unit successful result must be the exact empty object",
                              "Unit rejects a non-object result with the same exact contract diagnostic");
    }
} // namespace

int main() {
    tests::support::TestResult testResult;

    testValidThreadAndRawPreservation(testResult);
    testRequiredAndOptionalFields(testResult);
    testOperationAndReadWrappers(testResult);
    testConversationUnionFoundation(testResult);
    testListPage(testResult);
    testOptionEncoding(testResult);
    testExactUnitContract(testResult);

    return testResult.processResult();
}
