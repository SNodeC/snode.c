/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/stdio/Client.h"
#include "ai/openai/codex/typed/Client.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace codex = ai::openai::codex;

    std::size_t descriptorCount() {
        std::size_t count = 0;
        for ([[maybe_unused]] const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator("/proc/self/fd")) {
            ++count;
        }
        return count;
    }

    bool contains(const std::vector<std::string>& values, const std::string& expected) {
        return std::find(values.begin(), values.end(), expected) != values.end();
    }

    std::size_t indexOf(const std::vector<std::string>& values, const std::string& expected) {
        const auto iterator = std::find(values.begin(), values.end(), expected);
        return iterator == values.end() ? values.size() : static_cast<std::size_t>(std::distance(values.begin(), iterator));
    }

    int runFlowScenario(char* argv[]) {
        tests::support::TestResult testResult;
        char* snodeArguments[] = {argv[0], nullptr};
        core::SNodeC::init(1, snodeArguments);

        const std::size_t descriptorsBefore = descriptorCount();
        codex::stdio::Client client(CODEX_FAKE_APP_SERVER, {"protocol-flow"});
        std::vector<codex::State> states;
        std::vector<std::string> diagnostics;
        std::vector<std::string> responseMethods;
        std::vector<std::string> notificationMethods;
        std::vector<codex::UnknownMessage> unknownMessages;
        std::map<std::string, codex::Json> expectedResults = {{"test/result-object", {{"shape", "object"}}},
                                                              {"test/result-array", codex::Json::array({1, "two", false})},
                                                              {"test/result-scalar", 17},
                                                              {"test/result-null", nullptr},
                                                              {"test/out-of-order/first", {{"order", "first"}}},
                                                              {"test/out-of-order/second", {{"order", "second"}}}};

        bool timedOut = false;
        bool failed = false;
        bool allInitialSubmissionsAccepted = true;
        bool turnSubmissionAccepted = false;
        bool insideSubmission = false;
        bool callbackWasInline = false;
        bool threadResponseValid = false;
        bool turnResponseValid = false;
        bool remoteErrorValid = false;
        bool resultShapesValid = true;
        bool futureNotificationPreserved = false;
        bool integerServerRequestPreserved = false;
        bool stringServerRequestPreserved = false;
        bool integerResponseAccepted = false;
        bool stringRejectionAccepted = false;
        bool turnCompleted = false;
        std::string streamedText;

        client.setOnDiagnostic([&diagnostics](const codex::Diagnostic& diagnostic) {
            diagnostics.push_back(diagnostic.message);
        });
        client.raw().setOnUnknownMessage([&unknownMessages](const codex::UnknownMessage& message) {
            unknownMessages.push_back(message);
        });
        client.raw().setOnNotification([&](const codex::Notification& notification) {
            notificationMethods.push_back(notification.method);
            if (notification.method == "future/notification") {
                futureNotificationPreserved =
                    notification.params == codex::Json::array({"preserved", 23}) &&
                    notification.raw == codex::Json{{"method", "future/notification"}, {"params", codex::Json::array({"preserved", 23})}};
            } else if (notification.method == "item/agentMessage/delta") {
                streamedText += notification.params.value("delta", "");
            } else if (notification.method == "turn/completed") {
                turnCompleted = notification.params.value("threadId", "") == "thread-fake-001" &&
                                notification.params["turn"].value("id", "") == "turn-fake-001" &&
                                notification.params["turn"].value("status", "") == "completed";
                client.stop();
            }
        });
        client.raw().setOnServerRequest([&](const codex::ServerRequest& request) {
            if (const auto integerId = std::get_if<std::int64_t>(&request.id.value())) {
                integerServerRequestPreserved = *integerId == 700 && request.method == "item/commandExecution/requestApproval" &&
                                                request.raw["id"] == 700 && request.params.value("itemId", "") == "command-700";
                const codex::AppServerClient::RawProtocol::SendResult result = client.raw().respond(request.id, {{"decision", "accept"}});
                integerResponseAccepted = static_cast<bool>(result);
            } else if (const auto stringId = std::get_if<std::string>(&request.id.value())) {
                stringServerRequestPreserved =
                    *stringId == "approval-string-701" && request.method == "item/commandExecution/requestApproval" &&
                    request.raw["id"] == "approval-string-701" && request.params.value("itemId", "") == "command-701";
                codex::ProtocolError error{-32000, "Request rejected", codex::Json{{"reason", "component-test"}}};
                const codex::AppServerClient::RawProtocol::SendResult result = client.raw().reject(request.id, std::move(error));
                stringRejectionAccepted = static_cast<bool>(result);
            }
        });

        const auto recordResponse = [&](const codex::Response& response) {
            callbackWasInline = callbackWasInline || insideSubmission;
            responseMethods.push_back(response.method);
            if (response.method == "test/remote-error") {
                remoteErrorValid = response.kind == codex::Response::Kind::RemoteError && response.remoteError &&
                                   response.remoteError->code == -32000 && response.remoteError->message == "fake remote failure" &&
                                   response.remoteError->data && *response.remoteError->data == codex::Json{{"reason", "example"}};
            } else if (const auto expected = expectedResults.find(response.method); expected != expectedResults.end()) {
                resultShapesValid =
                    resultShapesValid && response.kind == codex::Response::Kind::Result && response.result == expected->second;
            }
        };

        client.setOnStateChanged([&](const codex::StateChange& stateChange) {
            states.push_back(stateChange.current);
            if (stateChange.current == codex::State::Ready) {
                insideSubmission = true;
                const auto threadSubmission =
                    client.raw().request("thread/start", {{"cwd", "/tmp/project"}}, [&](const codex::Response& response) {
                        callbackWasInline = callbackWasInline || insideSubmission;
                        responseMethods.push_back(response.method);
                        threadResponseValid = response.kind == codex::Response::Kind::Result &&
                                              response.result["thread"].value("id", "") == "thread-fake-001" &&
                                              response.result.value("cwd", "") == "/tmp/project";
                        insideSubmission = true;
                        const auto turnSubmission = client.raw().request(
                            "turn/start",
                            {{"threadId", "thread-fake-001"},
                             {"input", codex::Json::array({codex::Json{{"type", "text"}, {"text", "Analyse the current branch."}}})}},
                            [&](const codex::Response& turnResponse) {
                                callbackWasInline = callbackWasInline || insideSubmission;
                                responseMethods.push_back(turnResponse.method);
                                turnResponseValid = turnResponse.kind == codex::Response::Kind::Result &&
                                                    turnResponse.result["turn"].value("id", "") == "turn-fake-001" &&
                                                    turnResponse.result["turn"].value("status", "") == "inProgress";
                            });
                        turnSubmissionAccepted = static_cast<bool>(turnSubmission);
                        insideSubmission = false;
                    });
                allInitialSubmissionsAccepted = allInitialSubmissionsAccepted && static_cast<bool>(threadSubmission);

                const std::vector<std::string> methods = {"test/result-object",
                                                          "test/result-array",
                                                          "test/result-scalar",
                                                          "test/result-null",
                                                          "test/remote-error",
                                                          "test/out-of-order/first",
                                                          "test/out-of-order/second"};
                for (const std::string& method : methods) {
                    const auto submission = client.raw().request(method, codex::Json::object(), recordResponse);
                    allInitialSubmissionsAccepted = allInitialSubmissionsAccepted && static_cast<bool>(submission);
                }
                insideSubmission = false;
            } else if (stateChange.current == codex::State::Failed) {
                failed = true;
                client.stop();
            } else if (stateChange.current == codex::State::Stopped) {
                core::SNodeC::stop();
            }
        });

        [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
            [&]() {
                timedOut = true;
                client.stop();
                core::SNodeC::stop();
            },
            utils::Timeval({6, 0}));

        client.start();
        const int startResult = core::SNodeC::start(utils::Timeval({7, 0}));
        const std::size_t descriptorsAfter = descriptorCount();

        const std::vector<codex::State> expectedStates = {
            codex::State::Starting, codex::State::Initializing, codex::State::Ready, codex::State::Stopping, codex::State::Stopped};
        testResult.expectTrue(!timedOut, "raw stdio protocol flow completes before the watchdog");
        testResult.expectEqual(0, startResult, "raw stdio protocol flow stops the event loop cleanly");
        testResult.expectTrue(!failed && states == expectedStates, "raw stdio protocol flow preserves the clean lifecycle");
        testResult.expectTrue(allInitialSubmissionsAccepted && turnSubmissionAccepted,
                              "thread/start, turn/start, and generic raw requests are accepted");
        testResult.expectTrue(!callbackWasInline, "stdio response callbacks never execute inline from raw request submission");
        testResult.expectTrue(threadResponseValid && turnResponseValid,
                              "schema-shaped thread/start and turn/start responses retain their raw results");
        testResult.expectTrue(remoteErrorValid, "remote protocol errors retain code, message, and arbitrary data");
        testResult.expectTrue(resultShapesValid, "object, array, scalar, and null results are delivered unchanged");
        const std::vector<std::string> expectedResponseMethods = {"test/out-of-order/second",
                                                                  "test/out-of-order/first",
                                                                  "test/remote-error",
                                                                  "test/result-null",
                                                                  "test/result-scalar",
                                                                  "test/result-array",
                                                                  "test/result-object",
                                                                  "thread/start",
                                                                  "turn/start"};
        testResult.expectTrue(responseMethods == expectedResponseMethods && indexOf(responseMethods, "test/out-of-order/second") <
                                                                                indexOf(responseMethods, "test/out-of-order/first"),
                              "out-of-order responses complete exactly the matching requests in wire order");
        testResult.expectTrue(futureNotificationPreserved && contains(notificationMethods, "future/notification"),
                              "an unknown future notification method and complete envelope are delivered");
        testResult.expectTrue(streamedText == "Analysis complete.", "streamed raw notification deltas preserve wire ordering");
        const std::vector<std::string> expectedNotificationMethods = {"future/notification",
                                                                      "thread/started",
                                                                      "turn/started",
                                                                      "item/agentMessage/delta",
                                                                      "item/agentMessage/delta",
                                                                      "turn/completed"};
        testResult.expectTrue(notificationMethods == expectedNotificationMethods,
                              "unknown and current notifications are delivered in exact incoming wire order");
        testResult.expectTrue(integerServerRequestPreserved && integerResponseAccepted,
                              "an integer server-request ID is preserved and answered successfully");
        testResult.expectTrue(stringServerRequestPreserved && stringRejectionAccepted,
                              "a string server-request ID is preserved and rejected with a protocol error");
        testResult.expectTrue(turnCompleted, "turn/completed is observed after streamed events and server-request answers");
        testResult.expectTrue(unknownMessages.size() == 2 && unknownMessages[0].raw["id"] == 987654 &&
                                  unknownMessages[1].raw["id"] == "future-response-id",
                              "uncorrelated integer and string responses are retained as unknown messages");
        testResult.expectTrue(diagnostics == std::vector<std::string>{"protocol-initialized"},
                              "the fake app-server diagnostic channel remains independent of protocol JSON");
        testResult.expectTrue(descriptorsAfter == descriptorsBefore, "protocol-flow stop reaps the child and restores descriptors");

        core::SNodeC::free();
        return testResult.processResult();
    }

    int runTypedFlowScenario(char* argv[]) {
        namespace typed = codex::typed;

        tests::support::TestResult testResult;
        char* snodeArguments[] = {argv[0], nullptr};
        core::SNodeC::init(1, snodeArguments);

        const std::size_t descriptorsBefore = descriptorCount();
        codex::stdio::Client client(CODEX_FAKE_APP_SERVER, {"typed-flow"});
        bool timedOut = false;
        bool failed = false;
        bool inlineCallback = false;
        bool insideSubmission = false;
        bool submissionsAccepted = true;
        bool threadStartValid = false;
        bool threadResumeValid = false;
        bool threadListValid = false;
        bool threadReadValid = false;
        bool turnStartValid = false;
        bool turnInterruptValid = false;
        bool threadStarted = false;
        bool turnStarted = false;
        bool itemStarted = false;
        bool unknownItemSeen = false;
        bool itemCompleted = false;
        bool turnCompleted = false;
        bool unknownEventSeen = false;
        bool rawUnknownEventSeen = false;
        bool commandApprovalSeen = false;
        bool fileApprovalSeen = false;
        bool userInputSeen = false;
        bool unknownRequestSeen = false;
        bool allResponsesAccepted = true;
        bool secondResponseRejected = false;
        bool observersCoexist = true;
        std::size_t typedEventCount = 0;
        std::size_t rawEventCount = 0;
        std::size_t typedRequestCount = 0;
        std::size_t rawRequestCount = 0;
        std::string agentText;
        std::string commandOutput;

        client.typed().events().setOnEvent([&](const typed::Event& event) {
            ++typedEventCount;
            if (const auto* started = std::get_if<typed::ThreadStarted>(&event)) {
                threadStarted =
                    started->thread.id.value == "thread-fake-001" && started->thread.raw["preview"] == "Analyse the current branch.";
            } else if (const auto* started = std::get_if<typed::TurnStarted>(&event)) {
                turnStarted = started->turn.threadId.value == "thread-fake-001" && started->turn.id.value == "turn-fake-001";
            } else if (const auto* started = std::get_if<typed::ItemStarted>(&event)) {
                itemStarted = true;
                unknownItemSeen = unknownItemSeen || std::holds_alternative<typed::UnknownItem>(started->item);
            } else if (const auto* delta = std::get_if<typed::AgentMessageDelta>(&event)) {
                agentText += delta->text;
            } else if (const auto* delta = std::get_if<typed::CommandOutputDelta>(&event)) {
                commandOutput += delta->output;
            } else if (const auto* completed = std::get_if<typed::ItemCompleted>(&event)) {
                itemCompleted = std::holds_alternative<typed::CommandExecutionItem>(completed->item);
            } else if (const auto* completed = std::get_if<typed::TurnCompleted>(&event)) {
                turnCompleted = completed->turn.status.value == "completed";
                client.stop();
            } else if (std::holds_alternative<typed::UnknownEvent>(event)) {
                unknownEventSeen = true;
                throw std::runtime_error("intentional typed event callback failure");
            }
        });
        client.raw().setOnNotification([&](const codex::Notification& notification) {
            observersCoexist = observersCoexist && typedEventCount > rawEventCount;
            ++rawEventCount;
            rawUnknownEventSeen = rawUnknownEventSeen || notification.method == "future/event";
        });

        client.typed().requests().setOnRequest([&](const typed::TypedServerRequest& request) {
            ++typedRequestCount;
            if (const auto* approval = std::get_if<typed::CommandApprovalRequest>(&request)) {
                commandApprovalSeen =
                    approval->command == std::optional<std::string>("printf protocol-flow") && approval->startedAtMs == 2000;
                allResponsesAccepted =
                    allResponsesAccepted &&
                    static_cast<bool>(client.typed().requests().respond(*approval, typed::ApprovalDecision::accept()));
            } else if (const auto* approval = std::get_if<typed::FileChangeApprovalRequest>(&request)) {
                fileApprovalSeen = approval->reason == std::optional<std::string>("apply deterministic patch");
                allResponsesAccepted =
                    allResponsesAccepted &&
                    static_cast<bool>(client.typed().requests().respond(*approval, typed::ApprovalDecision::decline()));
            } else if (const auto* input = std::get_if<typed::UserInputRequest>(&request)) {
                userInputSeen =
                    input->questions.size() == 1 && input->questions.front().allowsFreeText && input->questions.front().options.size() == 1;
                allResponsesAccepted =
                    allResponsesAccepted &&
                    static_cast<bool>(client.typed().requests().respond(*input, {{"scope", {"Current"}}}));
            } else if (const auto* unknown = std::get_if<typed::UnknownServerRequest>(&request)) {
                unknownRequestSeen = unknown->method == "future/serverRequest" && unknown->params["future"] == true;
                allResponsesAccepted =
                    allResponsesAccepted &&
                    static_cast<bool>(client.typed().requests().respondRaw(*unknown, {{"handled", true}}));
            }
        });
        client.raw().setOnServerRequest([&](const codex::ServerRequest& request) {
            observersCoexist = observersCoexist && typedRequestCount > rawRequestCount;
            ++rawRequestCount;
            if (request.method == "item/commandExecution/requestApproval") {
                secondResponseRejected = !client.raw().respond(request.id, {{"decision", "cancel"}});
            }
        });

        client.setOnStateChanged([&](const codex::StateChange& stateChange) {
            if (stateChange.current == codex::State::Ready) {
                insideSubmission = true;
                const auto submission = client.typed().threads().start({.cwd = "/tmp/project"}, [&](const auto& startResult) {
                    inlineCallback = inlineCallback || insideSubmission;
                    threadStartValid = startResult && startResult.value->id.value == "thread-fake-001" && startResult.value->model &&
                                       startResult.value->model->value == "gpt-5" && startResult.raw["cwd"] == "/tmp/project";

                    insideSubmission = true;
                    const auto resumeSubmission =
                        client.typed().threads().resume(startResult.value->id, {.cwd = "/tmp/project"}, [&](const auto& resumeResult) {
                            inlineCallback = inlineCallback || insideSubmission;
                            threadResumeValid = resumeResult && resumeResult.value->id.value == "thread-fake-001";

                            insideSubmission = true;
                            const auto listSubmission =
                                client.typed().threads().list({.cursor = "cursor-1", .limit = 2}, [&](const auto& listResult) {
                                    inlineCallback = inlineCallback || insideSubmission;
                                    threadListValid = listResult && listResult.value->data.size() == 1 &&
                                                      listResult.value->nextCursor == std::optional<std::string>("cursor-2");

                                    insideSubmission = true;
                                    const auto readSubmission = client.typed().threads().read(
                                        typed::ThreadId{"thread-fake-001"}, {.includeTurns = true}, [&](const auto& readResult) {
                                            inlineCallback = inlineCallback || insideSubmission;
                                            threadReadValid = readResult && readResult.value->raw == readResult.raw["thread"];

                                            insideSubmission = true;
                                            const auto turnSubmission = client.typed().turns().start(
                                                typed::ThreadId{"thread-fake-001"},
                                                {typed::TextInput{"Analyse the current branch."}},
                                                {.reasoningEffort = typed::ReasoningEffort::high()},
                                                [&](const auto& turnResult) {
                                                    inlineCallback = inlineCallback || insideSubmission;
                                                    turnStartValid = turnResult && turnResult.value->threadId.value == "thread-fake-001" &&
                                                                     turnResult.value->status.value == "inProgress";

                                                    insideSubmission = true;
                                                    const auto interruptSubmission =
                                                        client.typed().turns().interrupt(typed::ThreadId{"thread-fake-001"},
                                                                                         typed::TurnId{"turn-fake-001"},
                                                                                         [&](const auto& interruptResult) {
                                                                                     inlineCallback = inlineCallback || insideSubmission;
                                                                                     turnInterruptValid =
                                                                                         static_cast<bool>(interruptResult);
                                                                                 });
                                                    submissionsAccepted = submissionsAccepted && static_cast<bool>(interruptSubmission);
                                                    insideSubmission = false;
                                                });
                                            submissionsAccepted = submissionsAccepted && static_cast<bool>(turnSubmission);
                                            insideSubmission = false;
                                        });
                                    submissionsAccepted = submissionsAccepted && static_cast<bool>(readSubmission);
                                    insideSubmission = false;
                                });
                            submissionsAccepted = submissionsAccepted && static_cast<bool>(listSubmission);
                            insideSubmission = false;
                        });
                    submissionsAccepted = submissionsAccepted && static_cast<bool>(resumeSubmission);
                    insideSubmission = false;
                });
                submissionsAccepted = submissionsAccepted && static_cast<bool>(submission);
                insideSubmission = false;
            } else if (stateChange.current == codex::State::Failed) {
                failed = true;
                client.stop();
            } else if (stateChange.current == codex::State::Stopped) {
                core::SNodeC::stop();
            }
        });

        [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
            [&]() {
                timedOut = true;
                client.stop();
                core::SNodeC::stop();
            },
            utils::Timeval({7, 0}));

        client.start();
        const int startResult = core::SNodeC::start(utils::Timeval({8, 0}));
        const std::size_t descriptorsAfter = descriptorCount();

        testResult.expectTrue(!timedOut && !failed, "typed stdio workflow completes cleanly before the watchdog");
        testResult.expectEqual(0, startResult, "typed stdio workflow stops the event loop cleanly");
        testResult.expectTrue(submissionsAccepted && !inlineCallback,
                              "typed operation submissions are accepted and every completion remains asynchronous");
        testResult.expectTrue(threadStartValid && threadResumeValid && threadListValid && threadReadValid,
                              "typed thread start, resume, list, and read results decode without application JSON handling");
        testResult.expectTrue(turnStartValid && turnInterruptValid,
                              "typed text turn start and turn interrupt results decode without application JSON handling");
        testResult.expectTrue(threadStarted && turnStarted && itemStarted && itemCompleted && turnCompleted,
                              "typed lifecycle event variants cover the complete fake workflow");
        testResult.expectTrue(agentText == "Analysis complete." && commandOutput == "typed-flow",
                              "typed agent-message and command-output deltas preserve streamed text");
        testResult.expectTrue(unknownEventSeen && rawUnknownEventSeen && unknownItemSeen,
                              "future notifications and item discriminators remain observable and nonfatal");
        testResult.expectTrue(commandApprovalSeen && fileApprovalSeen && userInputSeen && unknownRequestSeen && allResponsesAccepted,
                              "typed approvals, user input, and raw unknown-request escape all use the shared response registry");
        testResult.expectTrue(secondResponseRejected,
                              "a second raw answer is rejected after the typed response consumes server-request ownership");
        testResult.expectTrue(observersCoexist && rawEventCount > 0 && rawRequestCount == 4,
                              "typed observers run before coexisting raw observers for the same incoming messages");
        testResult.expectTrue(descriptorsAfter == descriptorsBefore, "typed workflow stop reaps the child and restores descriptors");

        core::SNodeC::free();
        return testResult.processResult();
    }

    int runExitRestartScenario(char* argv[]) {
        tests::support::TestResult testResult;
        std::string markerTemplate = (std::filesystem::temp_directory_path() / "snodec-codex-protocol-exit-XXXXXX").string();
        const int reservedMarker = ::mkstemp(markerTemplate.data());
        testResult.expectTrue(reservedMarker >= 0, "restart test reserves a unique marker path");
        if (reservedMarker < 0) {
            return testResult.processResult();
        }
        ::close(reservedMarker);
        if (::unlink(markerTemplate.c_str()) != 0) {
            testResult.expectTrue(false, "restart marker path is clear before the first launch");
            return testResult.processResult();
        }

        char* snodeArguments[] = {argv[0], nullptr};
        core::SNodeC::init(1, snodeArguments);
        const std::size_t descriptorsBefore = descriptorCount();
        codex::stdio::Client client(CODEX_FAKE_APP_SERVER, {"protocol-exit-once", markerTemplate});
        std::vector<codex::State> states;
        std::vector<std::string> diagnostics;
        std::optional<codex::ClientRequestId> firstId;
        std::optional<codex::ClientRequestId> secondId;
        bool timedOut = false;
        bool firstAccepted = false;
        bool secondAccepted = false;
        bool firstCancelled = false;
        bool secondCompleted = false;
        bool staleResponsePreserved = false;
        int cancellationCount = 0;
        int failureCount = 0;
        int readyCount = 0;
        int stoppedCount = 0;

        client.setOnDiagnostic([&](const codex::Diagnostic& diagnostic) {
            diagnostics.push_back(diagnostic.message);
        });
        client.raw().setOnUnknownMessage([&](const codex::UnknownMessage& message) {
            staleResponsePreserved =
                firstId && message.raw.value("id", -1LL) == firstId->value() && message.raw["result"].value("generation", "") == "stale";
        });
        client.setOnStateChanged([&](const codex::StateChange& stateChange) {
            states.push_back(stateChange.current);
            if (stateChange.current == codex::State::Ready) {
                ++readyCount;
                if (readyCount == 1) {
                    const auto submission =
                        client.raw().request("test/pending-after-exit", codex::Json::object(), [&](const codex::Response& response) {
                            ++cancellationCount;
                            firstCancelled = response.kind == codex::Response::Kind::Cancelled && response.localError &&
                                             response.localError->category == codex::Error::Category::Cancelled &&
                                             response.method == "test/pending-after-exit";
                        });
                    firstAccepted = static_cast<bool>(submission);
                    firstId = submission.id;
                } else if (readyCount == 2) {
                    const auto submission =
                        client.raw().request("test/pending-after-exit", codex::Json::object(), [&](const codex::Response& response) {
                            secondCompleted =
                                response.kind == codex::Response::Kind::Result && response.result.value("generation", "") == "current";
                            client.stop();
                        });
                    secondAccepted = static_cast<bool>(submission);
                    secondId = submission.id;
                }
            } else if (stateChange.current == codex::State::Failed) {
                ++failureCount;
                client.stop();
            } else if (stateChange.current == codex::State::Stopped) {
                ++stoppedCount;
                if (stoppedCount == 1) {
                    client.start();
                } else {
                    core::SNodeC::stop();
                }
            }
        });

        [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
            [&]() {
                timedOut = true;
                client.stop();
                core::SNodeC::stop();
            },
            utils::Timeval({7, 0}));

        client.start();
        const int startResult = core::SNodeC::start(utils::Timeval({8, 0}));
        const std::size_t descriptorsAfter = descriptorCount();
        const bool markerRemoved = ::unlink(markerTemplate.c_str()) == 0;

        const std::vector<codex::State> expectedStates = {codex::State::Starting,
                                                          codex::State::Initializing,
                                                          codex::State::Ready,
                                                          codex::State::Failed,
                                                          codex::State::Stopping,
                                                          codex::State::Stopped,
                                                          codex::State::Starting,
                                                          codex::State::Initializing,
                                                          codex::State::Ready,
                                                          codex::State::Stopping,
                                                          codex::State::Stopped};
        testResult.expectTrue(!timedOut, "exit/cancellation/restart flow completes before the watchdog");
        testResult.expectEqual(0, startResult, "restart flow stops the event loop cleanly");
        testResult.expectTrue(states == expectedStates && failureCount == 1 && readyCount == 2 && stoppedCount == 2,
                              "an unexpected child exit follows the explicit Failed-stop-Stopped restart sequence");
        testResult.expectTrue(firstAccepted && firstCancelled && cancellationCount == 1,
                              "a request pending at process exit is cancelled exactly once");
        testResult.expectTrue(secondAccepted && secondCompleted, "the same stdio client completes a request after explicit restart");
        testResult.expectTrue(firstId && secondId && secondId->value() > firstId->value(),
                              "client request IDs remain monotonic across restart");
        testResult.expectTrue(staleResponsePreserved, "an old-generation response is unknown and cannot complete the restarted request");
        testResult.expectTrue(diagnostics == std::vector<std::string>{"protocol-initialized", "protocol-initialized"},
                              "each app-server generation has an independently framed diagnostic stream");
        testResult.expectTrue(markerRemoved, "restart test removes its temporary generation marker");
        testResult.expectTrue(descriptorsAfter == descriptorsBefore, "restart stop reaps both children and restores descriptors");

        core::SNodeC::free();
        return testResult.processResult();
    }
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult testResult;
    if (argc != 2) {
        testResult.expectTrue(false, "exactly one protocol-flow scenario argument is required");
        return testResult.processResult();
    }

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("CodexStdioProtocolFlowTest");
        return tests::support::cTestSkipReturnCode;
    }

    const std::string scenario = argv[1];
    if (scenario == "flow") {
        return runFlowScenario(argv);
    }
    if (scenario == "typed-flow") {
        return runTypedFlowScenario(argv);
    }
    if (scenario == "exit-restart") {
        return runExitRestartScenario(argv);
    }

    testResult.expectTrue(false, "unknown protocol-flow scenario: " + scenario);
    return testResult.processResult();
}
