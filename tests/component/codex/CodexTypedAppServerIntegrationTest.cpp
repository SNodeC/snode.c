/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/stdio/Client.h"
#include "ai/openai/codex/typed/Client.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    constexpr std::string_view integrationGate = "SNODEC_RUN_CODEX_TYPED_INTEGRATION";

    int skip(const std::string& reason) {
        std::cout << "SKIP: " << reason << std::endl;
        return tests::support::cTestSkipReturnCode;
    }

    bool executableOnPath(const std::string_view executable) {
        const char* path = std::getenv("PATH");
        if (path == nullptr) {
            return false;
        }

        const std::string searchPath(path);
        std::size_t start = 0;
        while (start <= searchPath.size()) {
            const std::size_t separator = searchPath.find(':', start);
            const std::string directory = searchPath.substr(start, separator - start);
            const std::filesystem::path candidate =
                (directory.empty() ? std::filesystem::path(".") : std::filesystem::path(directory)) / executable;
            std::error_code error;
            const std::filesystem::perms permissions = std::filesystem::status(candidate, error).permissions();
            if (!error && std::filesystem::is_regular_file(candidate, error) &&
                (permissions & (std::filesystem::perms::owner_exec | std::filesystem::perms::group_exec |
                                std::filesystem::perms::others_exec)) != std::filesystem::perms::none) {
                return true;
            }
            if (separator == std::string::npos) {
                break;
            }
            start = separator + 1;
        }

        return false;
    }

    std::string describeRemoteError(const std::optional<ai::openai::codex::ProtocolError>& error) {
        if (!error.has_value()) {
            return "remote error without protocol-error details";
        }

        return "remote error " + std::to_string(error->code) + ": " + error->message;
    }

    std::string describeLocalError(const std::optional<ai::openai::codex::Error>& error) {
        if (!error.has_value()) {
            return "local error without details";
        }

        return "local error " + std::to_string(error->code) + ": " + error->message;
    }
} // namespace

int main(int argc, char* argv[]) {
    namespace codex = ai::openai::codex;
    namespace typed = ai::openai::codex::typed;

    const char* gateValue = std::getenv(integrationGate.data());
    if (gateValue == nullptr || std::string_view(gateValue) != "1") {
        return skip("set " + std::string(integrationGate) +
                    "=1 to run the real typed Codex App Server integration; the test may use configured credentials and quota");
    }
    if (!executableOnPath("codex")) {
        return skip("codex executable is not available on PATH");
    }
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("CodexTypedAppServerIntegrationTest");
        return tests::support::cTestSkipReturnCode;
    }

    std::error_code filesystemError;
    const std::filesystem::path temporaryBase = std::filesystem::temp_directory_path(filesystemError);
    if (filesystemError) {
        return skip("unable to locate a temporary directory for the isolated typed App Server workspace: " + filesystemError.message());
    }

    std::string workspaceTemplate = (temporaryBase / "snodec-codex-typed-integration-XXXXXX").string();
    const char* const createdWorkspace = ::mkdtemp(workspaceTemplate.data());
    if (createdWorkspace == nullptr) {
        return skip("unable to create an isolated temporary working directory for the typed App Server workflow");
    }
    const std::filesystem::path workspace(createdWorkspace);

    // Keep rollout and state-database writes away from the user's project while
    // retaining the configured CODEX_HOME so an explicitly enabled run can use
    // the caller's existing authentication.
    if (::setenv("CODEX_SQLITE_HOME", workspace.c_str(), 1) != 0) {
        std::filesystem::remove_all(workspace, filesystemError);
        return skip("unable to direct the Codex state database to the isolated temporary workspace");
    }

    core::SNodeC::init(argc, argv);

    tests::support::TestResult testResult;
    codex::stdio::Client client;
    std::vector<codex::State> states;
    std::vector<std::string> diagnostics;
    std::string workflowFailure;
    std::string runtimeSkipReason;
    std::string turnOutcomeDescription;
    std::string unknownEventDiagnostic;
    bool stopRequested = false;
    bool timedOut = false;
    bool lifecycleFailed = false;
    bool stopped = false;
    bool threadStartAttempted = false;
    bool threadSubmissionAccepted = false;
    bool threadStartSucceeded = false;
    bool turnStartAttempted = false;
    bool turnSubmissionAccepted = false;
    bool turnResultCallbackSeen = false;
    bool turnStartSucceeded = false;
    bool turnRemoteError = false;
    bool terminalTurnEvent = false;
    bool approvalResponseFailed = false;
    std::size_t typedEventCount = 0;
    std::size_t knownTypedEventCount = 0;
    std::size_t approvalRequestsSeen = 0;
    std::size_t approvalRequestsDeclined = 0;

    const auto requestStop = [&]() {
        if (!stopRequested) {
            stopRequested = true;
            client.stop();
        }
    };
    const auto failWorkflow = [&](std::string message) {
        if (workflowFailure.empty()) {
            workflowFailure = std::move(message);
        }
        requestStop();
    };
    const auto skipRuntime = [&](std::string message) {
        if (runtimeSkipReason.empty()) {
            runtimeSkipReason = std::move(message);
        }
        requestStop();
    };

    client.setOnDiagnostic([&](const codex::Diagnostic& diagnostic) {
        diagnostics.push_back(diagnostic.message);
    });

    client.typed().events().setOnEvent([&](const typed::Event& event) {
        ++typedEventCount;

        if (const auto* unknown = std::get_if<typed::UnknownEvent>(&event)) {
            if (unknown->decodingError.has_value() && unknownEventDiagnostic.empty()) {
                unknownEventDiagnostic = unknown->method + ": " + *unknown->decodingError;
            }
            return;
        }

        ++knownTypedEventCount;
        if (const auto* completed = std::get_if<typed::TurnCompleted>(&event)) {
            terminalTurnEvent = true;
            turnOutcomeDescription = "typed turn/completed with status " + completed->turn.status.value;
            if (turnResultCallbackSeen) {
                requestStop();
            }
        } else if (const auto* failed = std::get_if<typed::TurnFailed>(&event)) {
            terminalTurnEvent = true;
            turnOutcomeDescription = "typed turn/completed classified as TurnFailed with status " + failed->turn.status.value;
            if (turnResultCallbackSeen) {
                requestStop();
            }
        }
    });

    client.typed().requests().setOnRequest([&](const typed::TypedServerRequest& request) {
        if (const auto* command = std::get_if<typed::CommandApprovalRequest>(&request)) {
            ++approvalRequestsSeen;
            const auto response = client.typed().requests().respond(*command, typed::ApprovalDecision::decline());
            if (response) {
                ++approvalRequestsDeclined;
            } else {
                approvalResponseFailed = true;
                failWorkflow("unable to decline a real command approval request: " + describeLocalError(response.error));
            }
        } else if (const auto* fileChange = std::get_if<typed::FileChangeApprovalRequest>(&request)) {
            ++approvalRequestsSeen;
            const auto response = client.typed().requests().respond(*fileChange, typed::ApprovalDecision::decline());
            if (response) {
                ++approvalRequestsDeclined;
            } else {
                approvalResponseFailed = true;
                failWorkflow("unable to decline a real file-change approval request: " + describeLocalError(response.error));
            }
        } else if (const auto* authentication = std::get_if<typed::AuthenticationRequest>(&request)) {
            skipRuntime("real codex app-server requested authentication during the typed workflow: " + authentication->reason);
        } else if (const auto* userInput = std::get_if<typed::UserInputRequest>(&request)) {
            skipRuntime("real codex app-server requested interactive user input (" + std::to_string(userInput->questions.size()) +
                        " question(s)); the optional test does not answer it automatically");
        } else if (const auto* unknown = std::get_if<typed::UnknownServerRequest>(&request)) {
            skipRuntime("real codex app-server sent unsupported server request '" + unknown->method +
                        "'; the optional test left it unanswered");
        }
    });

    client.setOnStateChanged([&](const codex::StateChange& stateChange) {
        states.push_back(stateChange.current);

        if (stateChange.current == codex::State::Ready) {
            typed::ThreadStartOptions options;
            options.cwd = workspace.string();
            options.approvalPolicy = typed::ApprovalPolicy::never();
            options.sandboxMode = typed::SandboxMode::readOnly();
            options.ephemeral = true;

            threadStartAttempted = true;
            const auto submission =
                client.typed().threads().start(std::move(options), [&](const typed::OperationResult<typed::Thread>& result) {
                using Result = typed::OperationResult<typed::Thread>;

                if (result.kind == Result::Kind::RemoteError) {
                    skipRuntime("typed thread/start was unavailable: " + describeRemoteError(result.remoteError));
                    return;
                }
                if (result.kind == Result::Kind::Cancelled && !runtimeSkipReason.empty()) {
                    return;
                }
                if (!result) {
                    failWorkflow("typed thread/start did not produce a typed thread: " + describeLocalError(result.localError));
                    return;
                }

                threadStartSucceeded = true;
                typed::TurnStartOptions turnOptions;
                turnOptions.cwd = workspace.string();
                turnOptions.approvalPolicy = typed::ApprovalPolicy::never();
                turnOptions.sandboxPolicy = typed::ReadOnlySandboxPolicy{false};

                turnStartAttempted = true;
                const auto turnSubmission = client.typed().turns().start(
                    result.value->id,
                    {typed::TextInput{"Reply with exactly OK. Do not use tools and do not modify any files."}},
                    std::move(turnOptions),
                    [&](const typed::OperationResult<typed::Turn>& turnResult) {
                        using TurnResult = typed::OperationResult<typed::Turn>;

                        turnResultCallbackSeen = true;
                        if (turnResult.kind == TurnResult::Kind::RemoteError) {
                            turnRemoteError = true;
                            turnOutcomeDescription = describeRemoteError(turnResult.remoteError);
                            requestStop();
                            return;
                        }
                        if (turnResult.kind == TurnResult::Kind::Cancelled && !runtimeSkipReason.empty()) {
                            return;
                        }
                        if (!turnResult) {
                            failWorkflow("typed turn/start did not produce a typed turn: " + describeLocalError(turnResult.localError));
                            return;
                        }

                        turnStartSucceeded = true;
                        if (terminalTurnEvent) {
                            requestStop();
                        }
                    });
                turnSubmissionAccepted = static_cast<bool>(turnSubmission);
                if (!turnSubmissionAccepted) {
                    failWorkflow("typed turn/start submission was rejected locally: " + describeLocalError(turnSubmission.error));
                }
            });
            threadSubmissionAccepted = static_cast<bool>(submission);
            if (!threadSubmissionAccepted) {
                failWorkflow("typed thread/start submission was rejected locally: " + describeLocalError(submission.error));
            }
        } else if (stateChange.current == codex::State::Failed) {
            lifecycleFailed = true;
            if (stateChange.error.has_value() && workflowFailure.empty()) {
                workflowFailure = "real App Server lifecycle failed: " + describeLocalError(stateChange.error);
            }
            requestStop();
        } else if (stateChange.current == codex::State::Stopped) {
            stopped = true;
            core::SNodeC::stop();
        }
    });

    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&]() {
            timedOut = true;
            if (workflowFailure.empty()) {
                workflowFailure = "real typed App Server workflow exceeded its 45-second watchdog";
            }
            requestStop();
        },
        utils::Timeval({45, 0}));

    client.start();
    const int startResult = core::SNodeC::start(utils::Timeval({50, 0}));

    const bool cleanStop = !timedOut && !lifecycleFailed && stopped && startResult == 0 && workflowFailure.empty();
    const std::string lastDiagnostic = diagnostics.empty() ? std::string() : "; last stderr diagnostic: " + diagnostics.back();
    const std::vector<codex::State> expectedStates = {
        codex::State::Starting, codex::State::Initializing, codex::State::Ready, codex::State::Stopping, codex::State::Stopped};

    if (runtimeSkipReason.empty() || !cleanStop) {
        testResult.expectTrue(!timedOut, "real typed workflow completes before the watchdog: " + workflowFailure + lastDiagnostic);
        testResult.expectEqual(0, startResult, "real typed integration event loop stops cleanly");
        testResult.expectTrue(!lifecycleFailed && stopped,
                              "real App Server reaches a clean Stopped state: " + workflowFailure + lastDiagnostic);
        testResult.expectTrue(states == expectedStates, "real typed App Server preserves the complete clean lifecycle order");
        testResult.expectTrue(threadStartAttempted && threadSubmissionAccepted && threadStartSucceeded,
                              "real typed thread/start submits and decodes without application JSON: " + workflowFailure);
        testResult.expectTrue(turnStartAttempted && turnSubmissionAccepted && (turnStartSucceeded || turnRemoteError),
                              "real typed turn/start submits and yields typed success or a classified remote error: " +
                                  turnOutcomeDescription);
        testResult.expectTrue(typedEventCount > 0 && knownTypedEventCount > 0,
                              "real workflow emits at least one recognized typed streamed event: " + unknownEventDiagnostic);
        testResult.expectTrue(terminalTurnEvent || (turnRemoteError && !turnOutcomeDescription.empty()),
                              "real turn reaches typed completion/failure or a clearly classified remote error: " + turnOutcomeDescription);
        testResult.expectTrue(!approvalResponseFailed && approvalRequestsSeen == approvalRequestsDeclined,
                              "real integration answers every command or file-change request only with decline; observed " +
                                  std::to_string(approvalRequestsSeen) + " and declined " + std::to_string(approvalRequestsDeclined));
        testResult.expectTrue(workflowFailure.empty(), "real typed workflow has no local or lifecycle failure: " + workflowFailure);
    }

    core::SNodeC::free();
    std::filesystem::remove_all(workspace, filesystemError);

    if (!runtimeSkipReason.empty() && cleanStop) {
        return skip(runtimeSkipReason);
    }

    return testResult.processResult();
}
