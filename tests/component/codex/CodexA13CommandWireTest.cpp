/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/detail/Transport.h"
#include "ai/openai/codex/typed/Client.h"
#include "ai/openai/codex/typed/Commands.h"
#include "ai/openai/codex/typed/Events.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include <variant>
#include <vector>

namespace {
    namespace codex = ai::openai::codex;
    namespace detail = ai::openai::codex::detail;
    namespace typed = ai::openai::codex::typed;

    using Submission = codex::AppServerClient::RawProtocol::Submission;
    constexpr std::size_t OperationCount = 4;

    bool writeFully(int descriptor, std::string_view bytes) {
        std::size_t offset = 0;
        while (offset < bytes.size()) {
            const ssize_t written = ::write(descriptor, bytes.data() + offset, bytes.size() - offset);
            if (written > 0) {
                offset += static_cast<std::size_t>(written);
            } else if (written < 0 && errno == EINTR) {
                continue;
            } else {
                return false;
            }
        }
        return true;
    }

    std::optional<std::string> readLine(int descriptor) {
        std::string line;
        for (;;) {
            char byte = '\0';
            const ssize_t received = ::read(descriptor, &byte, 1);
            if (received == 1) {
                line.push_back(byte);
                if (byte == '\n') {
                    return line;
                }
            } else if (received < 0 && errno == EINTR) {
                continue;
            } else {
                return std::nullopt;
            }
        }
    }

    codex::Json initializeResult() {
        return {
            {"codexHome", "/tmp/codex-a1-3-command-wire"},
            {"platformFamily", "unix"},
            {"platformOs", "linux"},
            {"userAgent", "codex-a1-3-command-wire/1"},
        };
    }

    codex::Json execResult() {
        return {
            {"exitCode", 17},
            {"stderr", "synthetic-buffered-stderr"},
            {"stdout", "synthetic-buffered-stdout"},
            {"futureResultField", true},
        };
    }

    struct OutboundRecord {
        std::string line;
        codex::Json envelope;
    };

    struct UnixTransportState {
        enum class ReplyMode { Success, RemoteError, Hold };

        detail::TransportCallbacks callbacks;
        std::vector<detail::TransportCallbacks> callbackGenerations;
        std::vector<OutboundRecord> outbound;
        std::map<std::string, codex::Json> successResults;
        ReplyMode replyMode = ReplyMode::Success;
        int clientDescriptor = -1;
        int serverDescriptor = -1;
        bool emitExecOutput = false;
        bool duplicateExecResult = false;
        bool running = false;

        ~UnixTransportState() {
            if (clientDescriptor >= 0) {
                ::close(clientDescriptor);
            }
            if (serverDescriptor >= 0) {
                ::close(serverDescriptor);
            }
        }

        bool open() {
            int descriptors[2]{-1, -1};
            if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, descriptors) != 0) {
                return false;
            }
            clientDescriptor = descriptors[0];
            serverDescriptor = descriptors[1];
            return true;
        }

        bool injectWith(const detail::TransportCallbacks& target, const codex::Json& envelope) {
            const std::string serverLine = envelope.dump() + "\n";
            if (!writeFully(serverDescriptor, serverLine)) {
                return false;
            }
            const std::optional<std::string> received = readLine(clientDescriptor);
            if (!received || *received != serverLine || !target.onMessage) {
                return false;
            }
            target.onMessage(received->substr(0, received->size() - 1));
            return true;
        }

        bool injectCurrent(const codex::Json& envelope) {
            return injectWith(callbacks, envelope);
        }

        bool send(std::string message) {
            const std::string clientLine = message + "\n";
            if (!writeFully(clientDescriptor, clientLine)) {
                return false;
            }
            const std::optional<std::string> received = readLine(serverDescriptor);
            if (!received || *received != clientLine) {
                return false;
            }

            codex::Json envelope = codex::Json::parse(received->begin(), received->end() - 1, nullptr, false);
            if (envelope.is_discarded()) {
                return false;
            }
            outbound.push_back({*received, envelope});

            const auto method = envelope.find("method");
            if (method == envelope.end() || !method->is_string()) {
                return true;
            }
            if (*method == "initialize") {
                const auto id = envelope.find("id");
                return id != envelope.end() && injectCurrent({{"id", *id}, {"result", initializeResult()}});
            }
            if (*method == "initialized" || envelope.find("id") == envelope.end()) {
                return true;
            }
            if (replyMode == ReplyMode::Hold) {
                return true;
            }

            const std::string operation = *method;
            if (replyMode == ReplyMode::RemoteError) {
                return injectCurrent({{"id", envelope.at("id")},
                                      {"error",
                                       {{"code", -32'413},
                                        {"message", "synthetic command remote failure"},
                                        {"data", {{"operation", operation}}},
                                        {"futureErrorField", true}}}});
            }

            if (operation == "command/exec" && emitExecOutput) {
                if (!injectCurrent({{"jsonrpc", "2.0"},
                                    {"method", "command/exec/outputDelta"},
                                    {"params",
                                     {{"capReached", false},
                                      {"deltaBase64", "c3ludGhldGljLXN0ZG91dA=="},
                                      {"processId", "process-wire"},
                                      {"stream", "stdout"},
                                      {"futureParam", "stdout-extension"}}},
                                    {"futureEnvelopeOnly", "stdout-envelope"}}) ||
                    !injectCurrent({{"jsonrpc", "2.0"},
                                    {"method", "command/exec/outputDelta"},
                                    {"params",
                                     {{"capReached", true},
                                      {"deltaBase64", "c3ludGhldGljLXN0ZGVycg=="},
                                      {"processId", "process-wire"},
                                      {"stream", "stderr"},
                                      {"futureParam", "stderr-extension"}}},
                                    {"futureEnvelopeOnly", "stderr-envelope"}})) {
                    return false;
                }
            }

            const auto result = successResults.find(operation);
            if (result == successResults.end() || !injectCurrent({{"id", envelope.at("id")}, {"result", result->second}})) {
                return false;
            }
            if (operation == "command/exec" && duplicateExecResult) {
                return injectCurrent({{"id", envelope.at("id")}, {"result", result->second}});
            }
            return true;
        }
    };

    class UnixTranscriptTransport final : public detail::Transport {
    public:
        explicit UnixTranscriptTransport(std::shared_ptr<UnixTransportState> state)
            : state(std::move(state)) {
        }

        void setCallbacks(detail::TransportCallbacks callbacks) override {
            state->callbacks = std::move(callbacks);
            if (state->callbacks.onStarted || state->callbacks.onMessage || state->callbacks.onDiagnostic || state->callbacks.onError ||
                state->callbacks.onExited) {
                state->callbackGenerations.push_back(state->callbacks);
            }
        }

        void start() override {
            state->running = true;
            if (state->callbacks.onStarted) {
                state->callbacks.onStarted();
            }
        }

        bool send(std::string message) override {
            return state->send(std::move(message));
        }

        void stop() override {
            if (!std::exchange(state->running, false)) {
                return;
            }
            if (state->callbacks.onExited) {
                state->callbacks.onExited(detail::ProcessExit{true, 0, false, 0});
            }
        }

    private:
        std::shared_ptr<UnixTransportState> state;
    };

    class TestClient final : public codex::AppServerClient {
    public:
        explicit TestClient(const std::shared_ptr<UnixTransportState>& state)
            : AppServerClient(std::make_unique<UnixTranscriptTransport>(state),
                              {"codex_a1_3_command_wire_test", "Codex A1.3 Command Wire Test", "1"}) {
        }
    };

    enum class Phase { LocalRejection, Success, RemoteError, Cancellation, Generation };

    struct OperationCase {
        std::string method;
        detail::ClientRequestTarget target;
        detail::ResultContractKind resultKind;
        codex::Json expectedParams;
        codex::Json expectedResult;
        std::function<Submission(Phase)> invoke;
    };

    class CommandWireRunner {
    public:
        explicit CommandWireRunner(tests::support::TestResult& result)
            : result(result)
            , state(std::make_shared<UnixTransportState>())
            , socketReady(state->open())
            , client(std::make_unique<TestClient>(state)) {
            buildCases();
        }

        void start() {
            expect(socketReady, "A1.3 command wire test opens its AF_UNIX socketpair");
            if (!socketReady) {
                finished = true;
                core::SNodeC::stop();
                return;
            }

            client->typed().events().setOnEvent([this](const typed::Event& event) {
                if (const auto* output = std::get_if<typed::CommandExecOutputDeltaNotification>(&event)) {
                    const bool isStdout = output->processId.value == "process-wire" &&
                                          output->stream == typed::CommandExecOutputStream::stdoutStream() &&
                                          output->deltaBase64 == "c3ludGhldGljLXN0ZG91dA==" && !output->capReached &&
                                          output->raw.value("futureEnvelopeOnly", "") == "stdout-envelope";
                    const bool isStderr = output->processId.value == "process-wire" &&
                                          output->stream == typed::CommandExecOutputStream::stderrStream() &&
                                          output->deltaBase64 == "c3ludGhldGljLXN0ZGVycg==" && output->capReached &&
                                          output->raw.value("futureEnvelopeOnly", "") == "stderr-envelope";
                    eventOrder.push_back(isStdout ? "typed-stdout" : (isStderr ? "typed-stderr" : "typed-invalid"));
                    ++typedOutputEvents;
                    expect(isStdout || isStderr,
                           "typed command chunks retain exact process, stream, encoded delta, "
                           "cap flag, and raw identity");
                    expect(!std::holds_alternative<typed::CommandOutputDelta>(event),
                           "one-off output is not conflated with A1.1 item output");
                    if (isStdout && !reentrantSubmitted) {
                        reentrantSubmitted = true;
                        const Submission submission = client->typed().commands().write(
                            {
                                .processId = typed::CommandExecProcessId{"process-wire"},
                                .deltaBase64 = typed::OptionalNullable<std::string>::withValue("cmVlbnRyYW50LWlucHV0"),
                                .closeStdin = false,
                            },
                            [this](const typed::OperationResult<typed::Unit>& operation) {
                                expect(operation.kind == typed::OperationResult<typed::Unit>::Kind::Success && operation.value &&
                                           operation.raw == codex::Json::object(),
                                       "reentrant command write completes through the same "
                                       "RawProtocol");
                                eventOrder.push_back("reentrant-write-result");
                                ++reentrantCallbacks;
                                maybeCompleteSuccess();
                            });
                        expect(static_cast<bool>(submission) && submission.id && !submission.error,
                               "an output callback can submit a reentrant command write");
                        eventCallbackThrew = true;
                        throw std::runtime_error("intentional first command output callback failure");
                    }
                    maybeCompleteSuccess();
                }
                if (const auto* unknown = std::get_if<typed::UnknownEvent>(&event);
                    unknown && unknown->method == "command/exec/outputDelta") {
                    ++malformedTypedEvents;
                    expect(unknown->diagnostic && unknown->diagnostic->kind == typed::DecodeIssueKind::MalformedKnownPayload &&
                               unknown->diagnostic->severity == typed::DecodeIssueSeverity::ProtocolWarning &&
                               unknown->raw.at("params").at("deltaBase64") == 7,
                           "malformed command output remains observable and diagnosed");
                    maybeBeginRemoteErrors();
                }
            });
            client->raw().setOnNotification([this](const codex::Notification& notification) {
                if (notification.method != "command/exec/outputDelta") {
                    return;
                }
                const codex::Json stdoutParams{
                    {"capReached", false},
                    {"deltaBase64", "c3ludGhldGljLXN0ZG91dA=="},
                    {"processId", "process-wire"},
                    {"stream", "stdout"},
                    {"futureParam", "stdout-extension"},
                };
                const codex::Json stderrParams{
                    {"capReached", true},
                    {"deltaBase64", "c3ludGhldGljLXN0ZGVycg=="},
                    {"processId", "process-wire"},
                    {"stream", "stderr"},
                    {"futureParam", "stderr-extension"},
                };
                if (notification.params == stdoutParams || notification.params == stderrParams) {
                    eventOrder.push_back(notification.params == stdoutParams ? "raw-stdout" : "raw-stderr");
                    ++rawOutputEvents;
                    maybeCompleteSuccess();
                } else {
                    ++malformedRawEvents;
                    maybeBeginRemoteErrors();
                }
            });

            client->setOnStateChanged([this](const codex::StateChange& change) {
                if (change.current == codex::State::Ready) {
                    ++readyCount;
                    if (readyCount == 1) {
                        beginSuccess();
                    } else if (readyCount == 2) {
                        beginGeneration();
                    }
                } else if (change.current == codex::State::Failed) {
                    expect(false, "A1.3 command socket transcript must not fail");
                    client->stop();
                } else if (change.current == codex::State::Stopped) {
                    if (readyCount == 1) {
                        firstStopObserved = true;
                        maybeRestart();
                    } else if (readyCount == 2) {
                        finished = true;
                        core::SNodeC::stop();
                    }
                }
            });

            invokeLocalRejections();
            client->start();
        }

        bool isFinished() const noexcept {
            return finished;
        }

    private:
        void expect(bool condition, const std::string& description) {
            result.expectTrue(condition, description);
        }

        std::vector<std::string> expectedMethods() const {
            std::vector<std::string> methods;
            methods.reserve(cases.size());
            for (const OperationCase& operation : cases) {
                methods.push_back(operation.method);
            }
            return methods;
        }

        template <typename Result>
        bool typedRawMatches(const typed::OperationResult<Result>& operation, const codex::Json& expected) {
            if (!operation.value) {
                return false;
            }
            if constexpr (std::is_same_v<Result, typed::Unit>) {
                return expected == codex::Json::object();
            } else {
                return operation.value->raw == expected;
            }
        }

        template <typename Result>
        std::function<void(const typed::OperationResult<Result>&)> handler(std::string method, codex::Json expectedResult, Phase phase) {
            return [this, method = std::move(method), expectedResult = std::move(expectedResult), phase](
                       const typed::OperationResult<Result>& operation) {
                expect(!insideSubmission, method + " completion remains asynchronous");
                if (phase == Phase::LocalRejection) {
                    ++unexpectedLocalCallbacks;
                    expect(false, method + " synchronous local rejection invokes no callback");
                    return;
                }
                if (phase == Phase::Success || phase == Phase::Generation) {
                    expect(operation.kind == typed::OperationResult<Result>::Kind::Success && operation.value &&
                               operation.raw == expectedResult && typedRawMatches(operation, expectedResult),
                           method + " decodes its authoritative result and retains exact raw");
                    if constexpr (std::is_same_v<Result, typed::CommandExecResponse>) {
                        expect(operation.value && operation.value->exitCode == 17 &&
                                   operation.value->stdoutData == "synthetic-buffered-stdout" &&
                                   operation.value->stderrData == "synthetic-buffered-stderr",
                               "command/exec exposes its exact final buffered fields");
                    }
                    auto& order = phase == Phase::Success ? successOrder : generationOrder;
                    auto& count = phase == Phase::Success ? successCallbacks : generationCallbacks;
                    order.push_back(method);
                    if (phase == Phase::Success && method == "command/exec") {
                        eventOrder.push_back("exec-result");
                    }
                    ++count;
                    if (phase == Phase::Success) {
                        maybeCompleteSuccess();
                        if (method == "command/exec/terminate") {
                            completionCallbackThrew = true;
                            throw std::runtime_error("intentional command completion callback failure");
                        }
                    } else if (count == OperationCount) {
                        core::EventReceiver::atNextTick([this]() {
                            completeGeneration();
                        });
                    }
                    return;
                }
                if (phase == Phase::RemoteError) {
                    const codex::Json expectedError{
                        {"code", -32'413},
                        {"message", "synthetic command remote failure"},
                        {"data", {{"operation", method}}},
                        {"futureErrorField", true},
                    };
                    expect(operation.kind == typed::OperationResult<Result>::Kind::RemoteError && !operation.value &&
                               operation.remoteError && operation.remoteError->code == -32'413 &&
                               operation.remoteError->message == "synthetic command remote failure" &&
                               operation.remoteError->raw == expectedError && operation.raw.is_null(),
                           method + " retains the exact JSON-RPC error");
                    remoteOrder.push_back(method);
                    if (++remoteCallbacks == OperationCount) {
                        core::EventReceiver::atNextTick([this]() {
                            beginCancellation();
                        });
                    }
                    return;
                }

                expect(operation.kind == typed::OperationResult<Result>::Kind::Cancelled && !operation.value && operation.localError &&
                           operation.localError->category == codex::Error::Category::Cancelled,
                       method + " receives one cancellation at the connection boundary");
                cancellationOrder.push_back(method);
                ++cancellationCallbacks;
                maybeRestart();
            };
        }

        template <typename Result, typename Params, typename Submit>
        OperationCase makeOperation(std::string method,
                                    detail::ClientRequestTarget target,
                                    detail::ResultContractKind resultKind,
                                    Params params,
                                    codex::Json expectedParams,
                                    codex::Json expectedResult,
                                    Submit submit) {
            state->successResults.emplace(method, expectedResult);
            return {
                method,
                target,
                resultKind,
                std::move(expectedParams),
                expectedResult,
                [this, method, expectedResult, params = std::move(params), submit = std::move(submit)](Phase phase) mutable {
                    return submit(params, handler<Result>(method, expectedResult, phase));
                },
            };
        }

        void buildCases() {
            auto& commands = client->typed().commands();
            cases.push_back(makeOperation<typed::CommandExecResponse>(
                "command/exec",
                detail::ClientRequestTarget::CommandExec,
                detail::ResultContractKind::Concrete,
                typed::CommandExecParams{
                    .command = {"synthetic-command", "argument with spaces", "", "--exact"},
                    .cwd = typed::OptionalNullable<std::string>::explicitNull(),
                    .disableOutputCap = false,
                    .disableTimeout = true,
                    .env = typed::OptionalNullable<std::map<std::string, std::optional<std::string>>>::withValue({
                        {"SET", "synthetic-value"},
                        {"UNSET", std::nullopt},
                    }),
                    .outputBytesCap = typed::OptionalNullable<std::uint64_t>::withValue(4'096),
                    .processId = typed::OptionalNullable<typed::CommandExecProcessId>::withValue({"process-wire"}),
                    .sandboxPolicy = typed::OptionalNullable<typed::SandboxPolicy>::explicitNull(),
                    .size = typed::OptionalNullable<typed::CommandExecTerminalSize>::withValue({120, 40}),
                    .streamStdin = true,
                    .streamStdoutStderr = true,
                    .timeoutMs = typed::OptionalNullable<std::int64_t>::withValue(30'000),
                    .tty = true,
                },
                {
                    {"command", codex::Json::array({"synthetic-command", "argument with spaces", "", "--exact"})},
                    {"cwd", nullptr},
                    {"disableOutputCap", false},
                    {"disableTimeout", true},
                    {"env",
                     {
                         {"SET", "synthetic-value"},
                         {"UNSET", nullptr},
                     }},
                    {"outputBytesCap", 4'096},
                    {"processId", "process-wire"},
                    {"sandboxPolicy", nullptr},
                    {"size", {{"cols", 120}, {"rows", 40}}},
                    {"streamStdin", true},
                    {"streamStdoutStderr", true},
                    {"timeoutMs", 30'000},
                    {"tty", true},
                },
                execResult(),
                [&commands](auto params, auto resultHandler) {
                    return commands.exec(std::move(params), std::move(resultHandler));
                }));
            cases.push_back(makeOperation<typed::Unit>("command/exec/resize",
                                                       detail::ClientRequestTarget::CommandExecResize,
                                                       detail::ResultContractKind::Unit,
                                                       typed::CommandExecResizeParams{{"process-wire"}, {132, 48}},
                                                       {
                                                           {"processId", "process-wire"},
                                                           {"size", {{"cols", 132}, {"rows", 48}}},
                                                       },
                                                       codex::Json::object(),
                                                       [&commands](auto params, auto resultHandler) {
                                                           return commands.resize(std::move(params), std::move(resultHandler));
                                                       }));
            cases.push_back(makeOperation<typed::Unit>("command/exec/terminate",
                                                       detail::ClientRequestTarget::CommandExecTerminate,
                                                       detail::ResultContractKind::Unit,
                                                       typed::CommandExecTerminateParams{{"process-wire"}},
                                                       {{"processId", "process-wire"}},
                                                       codex::Json::object(),
                                                       [&commands](auto params, auto resultHandler) {
                                                           return commands.terminate(std::move(params), std::move(resultHandler));
                                                       }));
            cases.push_back(
                makeOperation<typed::Unit>("command/exec/write",
                                           detail::ClientRequestTarget::CommandExecWrite,
                                           detail::ResultContractKind::Unit,
                                           typed::CommandExecWriteParams{
                                               .processId = {"process-wire"},
                                               .deltaBase64 = typed::OptionalNullable<std::string>::withValue("c3ludGhldGljLWlucHV0"),
                                               .closeStdin = true,
                                           },
                                           {
                                               {"closeStdin", true},
                                               {"deltaBase64", "c3ludGhldGljLWlucHV0"},
                                               {"processId", "process-wire"},
                                           },
                                           codex::Json::object(),
                                           [&commands](auto params, auto resultHandler) {
                                               return commands.write(std::move(params), std::move(resultHandler));
                                           }));
        }

        void invokeLocalRejections() {
            std::size_t exact = 0;
            for (OperationCase& operation : cases) {
                insideSubmission = true;
                const Submission submission = operation.invoke(Phase::LocalRejection);
                insideSubmission = false;
                const detail::ProtocolSurfaceEntry& entry = detail::entryFor(operation.target);
                const detail::ProtocolSurfaceKey expectedKey{
                    detail::SurfaceCategory::ClientRequest,
                    "ClientRequest",
                    "method",
                    operation.method,
                };
                const auto* registeredTarget = std::get_if<detail::ClientRequestTarget>(&entry.runtimeTarget);
                const bool registryExact = entry.key == expectedKey && registeredTarget != nullptr &&
                                           *registeredTarget == operation.target &&
                                           entry.operationContract.resultKind == operation.resultKind;
                const bool rejected =
                    !submission && !submission.id && submission.error && submission.error->category == codex::Error::Category::InvalidState;
                exact += rejected && registryExact ? 1U : 0U;
                expect(rejected, operation.method + " rejects synchronously before RawProtocol is ready");
                expect(registryExact, operation.method + " starts from its exact registry target and result kind");
            }
            expect(exact == OperationCount && unexpectedLocalCallbacks == 0,
                   "all command operations reject locally without callbacks or traffic");
        }

        void invokeBatch(Phase phase) {
            const std::size_t before = state->outbound.size();
            std::vector<std::optional<codex::ClientRequestId>> ids;
            ids.reserve(cases.size());
            for (OperationCase& operation : cases) {
                insideSubmission = true;
                const Submission submission = operation.invoke(phase);
                insideSubmission = false;
                expect(static_cast<bool>(submission) && submission.id && !submission.error,
                       operation.method + " is accepted by the one RawProtocol engine");
                ids.push_back(submission.id);
            }
            expect(state->outbound.size() == before + cases.size(), "all four command operations cross the AF_UNIX transport once");
            for (std::size_t index = 0; index < cases.size(); ++index) {
                const codex::Json expectedEnvelope{
                    {"id", ids[index] ? codex::Json(ids[index]->value()) : codex::Json(nullptr)},
                    {"method", cases[index].method},
                    {"params", cases[index].expectedParams},
                };
                const OutboundRecord& record = state->outbound[before + index];
                expect(record.envelope == expectedEnvelope && record.line == expectedEnvelope.dump() + "\n",
                       cases[index].method + " sends exact JSON-RPC/JSONL bytes");
            }
            if (phase == Phase::Generation) {
                generationIds = std::move(ids);
            }
        }

        void beginSuccess() {
            state->replyMode = UnixTransportState::ReplyMode::Success;
            state->emitExecOutput = true;
            state->duplicateExecResult = true;
            invokeBatch(Phase::Success);
        }

        void maybeCompleteSuccess() {
            if (successCompletionScheduled || successCallbacks != OperationCount || reentrantCallbacks != 1 || typedOutputEvents != 2 ||
                rawOutputEvents != 2) {
                return;
            }
            successCompletionScheduled = true;
            core::EventReceiver::atNextTick([this]() {
                completeSuccess();
            });
        }

        void completeSuccess() {
            expect(successOrder == expectedMethods() && successCallbacks == OperationCount,
                   "command success callbacks complete once in submission order");
            expect(eventCallbackThrew && completionCallbackThrew && successOrder.back() == "command/exec/write",
                   "event and completion callback exceptions are contained");
            const auto typedStdout = std::find(eventOrder.begin(), eventOrder.end(), "typed-stdout");
            const auto typedStderr = std::find(eventOrder.begin(), eventOrder.end(), "typed-stderr");
            const auto rawStdout = std::find(eventOrder.begin(), eventOrder.end(), "raw-stdout");
            const auto rawStderr = std::find(eventOrder.begin(), eventOrder.end(), "raw-stderr");
            const auto execCompletion = std::find(eventOrder.begin(), eventOrder.end(), "exec-result");
            expect(typedStdout != eventOrder.end() && typedStderr != eventOrder.end() && rawStdout != eventOrder.end() &&
                       rawStderr != eventOrder.end() && execCompletion != eventOrder.end() && typedStdout < typedStderr &&
                       typedStderr < execCompletion && rawStdout < rawStderr && rawStderr < execCompletion,
                   "typed and raw observers preserve stdout-before-stderr order and both "
                   "chunks precede command/exec completion");
            expect(reentrantSubmitted && reentrantCallbacks == 1, "command output reentrancy produces one completion");

            const auto reentrant = std::find_if(state->outbound.begin(), state->outbound.end(), [](const OutboundRecord& record) {
                return record.envelope.value("method", "") == "command/exec/write" && record.envelope.contains("params") &&
                       record.envelope.at("params").value("deltaBase64", "") == "cmVlbnRyYW50LWlucHV0";
            });
            const codex::Json expectedReentrantParams{
                {"closeStdin", false},
                {"deltaBase64", "cmVlbnRyYW50LWlucHV0"},
                {"processId", "process-wire"},
            };
            expect(reentrant != state->outbound.end() && reentrant->envelope.at("params") == expectedReentrantParams &&
                       reentrant->line == reentrant->envelope.dump() + "\n",
                   "reentrant write uses the same exact JSONL path and process identity");

            state->emitExecOutput = false;
            state->duplicateExecResult = false;
            expect(state->injectCurrent(
                       {{"jsonrpc", "2.0"},
                        {"method", "command/exec/outputDelta"},
                        {"params", {{"capReached", false}, {"deltaBase64", 7}, {"processId", "process-wire"}, {"stream", "stdout"}}}}),
                   "malformed known command notification crosses the socket");
        }

        void maybeBeginRemoteErrors() {
            if (remoteScheduled || malformedTypedEvents != 1 || malformedRawEvents != 1) {
                return;
            }
            remoteScheduled = true;
            core::EventReceiver::atNextTick([this]() {
                beginRemoteErrors();
            });
        }

        void beginRemoteErrors() {
            state->replyMode = UnixTransportState::ReplyMode::RemoteError;
            invokeBatch(Phase::RemoteError);
        }

        void beginCancellation() {
            expect(remoteCallbacks == OperationCount && remoteOrder == expectedMethods(),
                   "all command JSON-RPC failures retain callback order");
            state->replyMode = UnixTransportState::ReplyMode::Hold;
            invokeBatch(Phase::Cancellation);
            client->stop();
        }

        void maybeRestart() {
            if (!restartScheduled && firstStopObserved && cancellationCallbacks == OperationCount) {
                restartScheduled = true;
                expect(cancellationOrder == expectedMethods(), "held command operations cancel exactly once in submission order");
                core::EventReceiver::atNextTick([this]() {
                    client->start();
                });
            }
        }

        void beginGeneration() {
            state->replyMode = UnixTransportState::ReplyMode::Hold;
            invokeBatch(Phase::Generation);
            expect(state->callbackGenerations.size() >= 2, "command restart installs a distinct transport generation");
            if (state->callbackGenerations.size() < 2) {
                client->stop();
                return;
            }

            const detail::TransportCallbacks stale = state->callbackGenerations.front();
            for (std::size_t index = 0; index < cases.size(); ++index) {
                if (generationIds[index]) {
                    expect(state->injectWith(stale, {{"id", generationIds[index]->value()}, {"result", cases[index].expectedResult}}),
                           cases[index].method + " stale response bytes cross the socket");
                }
            }
            core::EventReceiver::atNextTick([this]() {
                expect(generationCallbacks == 0, "stale command responses are rejected before delivery");
                const detail::TransportCallbacks current = state->callbackGenerations.back();
                for (std::size_t index = 0; index < cases.size(); ++index) {
                    if (generationIds[index]) {
                        expect(state->injectWith(current, {{"id", generationIds[index]->value()}, {"result", cases[index].expectedResult}}),
                               cases[index].method + " current response bytes cross the socket");
                    }
                }
            });
        }

        void completeGeneration() {
            expect(generationCallbacks == OperationCount && generationOrder == expectedMethods(),
                   "current-generation command responses complete once without "
                   "resurrecting stale callbacks");
            client->stop();
        }

        tests::support::TestResult& result;
        std::shared_ptr<UnixTransportState> state;
        bool socketReady = false;
        std::unique_ptr<TestClient> client;
        std::vector<OperationCase> cases;
        std::vector<std::string> eventOrder;
        std::vector<std::string> successOrder;
        std::vector<std::string> remoteOrder;
        std::vector<std::string> cancellationOrder;
        std::vector<std::string> generationOrder;
        std::vector<std::optional<codex::ClientRequestId>> generationIds;
        std::size_t successCallbacks = 0;
        std::size_t remoteCallbacks = 0;
        std::size_t cancellationCallbacks = 0;
        std::size_t generationCallbacks = 0;
        std::size_t reentrantCallbacks = 0;
        std::size_t typedOutputEvents = 0;
        std::size_t rawOutputEvents = 0;
        std::size_t malformedTypedEvents = 0;
        std::size_t malformedRawEvents = 0;
        std::size_t unexpectedLocalCallbacks = 0;
        int readyCount = 0;
        bool insideSubmission = false;
        bool reentrantSubmitted = false;
        bool eventCallbackThrew = false;
        bool completionCallbackThrew = false;
        bool successCompletionScheduled = false;
        bool remoteScheduled = false;
        bool firstStopObserved = false;
        bool restartScheduled = false;
        bool finished = false;
    };
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    core::SNodeC::init(argc, argv);

    CommandWireRunner runner(result);
    runner.start();

    bool timedOut = false;
    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&]() {
            timedOut = true;
            core::SNodeC::stop();
        },
        utils::Timeval({15, 0}));
    const int startResult = core::SNodeC::start(utils::Timeval({16, 0}));

    result.expectTrue(!timedOut && runner.isFinished(), "A1.3 command AF_UNIX lifecycle matrix completes before watchdog");
    result.expectEqual(0, startResult, "A1.3 command lifecycle matrix stops the event loop cleanly");
    core::SNodeC::free();
    return result.processResult();
}
