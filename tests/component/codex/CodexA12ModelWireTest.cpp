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
#include "ai/openai/codex/typed/Models.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <cstddef>
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
    constexpr std::size_t OperationCount = 2;

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
            {"codexHome", "/tmp/codex-a1-2-model-wire"},
            {"platformFamily", "unix"},
            {"platformOs", "linux"},
            {"userAgent", "codex-a1-2-model-wire/1"},
        };
    }

    codex::Json modelResult() {
        return {
            {"data",
             codex::Json::array(
                 {{{"defaultReasoningEffort", "medium"},
                   {"description", "Wire model"},
                   {"displayName", "Wire Model"},
                   {"hidden", false},
                   {"id", "model-wire-id"},
                   {"isDefault", true},
                   {"model", "model-wire"},
                   {"supportedReasoningEfforts",
                    codex::Json::array(
                        {{{"description", "Balanced"}, {"reasoningEffort", "medium"}}})}}})},
            {"nextCursor", nullptr},
            {"futureResult", true},
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
        bool injectObservers = false;
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
            if (injectObservers &&
                !injectCurrent({{"jsonrpc", "2.0"},
                                {"method", "model/verification"},
                                {"params",
                                 {{"threadId", "thread-observer"},
                                  {"turnId", "turn-observer"},
                                  {"verifications", codex::Json::array({"trustedAccessForCyber"})},
                                  {"operation", operation}}},
                                {"futureEnvelopeOnly", operation}})) {
                return false;
            }
            if (replyMode == ReplyMode::RemoteError) {
                return injectCurrent(
                    {{"id", envelope.at("id")},
                     {"error",
                      {{"code", -32'413},
                       {"message", "synthetic model remote failure"},
                       {"data", {{"operation", operation}}},
                       {"futureErrorField", true}}}});
            }

            const auto result = successResults.find(operation);
            return result != successResults.end() &&
                   injectCurrent({{"id", envelope.at("id")}, {"result", result->second}});
        }
    };

    class UnixTranscriptTransport final : public detail::Transport {
    public:
        explicit UnixTranscriptTransport(std::shared_ptr<UnixTransportState> state)
            : state(std::move(state)) {
        }

        void setCallbacks(detail::TransportCallbacks callbacks) override {
            state->callbacks = std::move(callbacks);
            if (state->callbacks.onStarted || state->callbacks.onMessage || state->callbacks.onDiagnostic ||
                state->callbacks.onError || state->callbacks.onExited) {
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
                              {"codex_a1_2_model_wire_test", "Codex A1.2 Model Wire Test", "1"}) {
        }
    };

    enum class Phase { LocalRejection, Success, RemoteError, Cancellation, Generation };

    struct OperationCase {
        std::string method;
        detail::ClientRequestTarget target;
        codex::Json expectedParams;
        codex::Json expectedResult;
        std::function<Submission(Phase)> invoke;
    };

    class ModelWireRunner {
    public:
        explicit ModelWireRunner(tests::support::TestResult& result)
            : result(result)
            , state(std::make_shared<UnixTransportState>())
            , socketReady(state->open())
            , client(std::make_unique<TestClient>(state)) {
            buildCases();
        }

        void start() {
            expect(socketReady, "A1.2 model wire test opens its AF_UNIX socketpair");
            if (!socketReady) {
                finished = true;
                core::SNodeC::stop();
                return;
            }

            client->typed().events().setOnEvent([this](const typed::Event& event) {
                const auto* verification = std::get_if<typed::ModelVerificationNotification>(&event);
                if (!verification || !verification->raw.contains("params")) {
                    return;
                }
                const codex::Json& params = verification->raw.at("params");
                if (!params.contains("operation") || !params.at("operation").is_string()) {
                    return;
                }
                const std::string method = params.at("operation");
                ++typedObserverCounts[method];
                observerOrder.push_back("typed:" + method);
            });
            client->raw().setOnNotification([this](const codex::Notification& notification) {
                if (notification.method != "model/verification" || !notification.params.is_object() ||
                    !notification.params.contains("operation") || !notification.params.at("operation").is_string()) {
                    return;
                }
                const std::string method = notification.params.at("operation");
                ++rawObserverCounts[method];
                observerOrder.push_back("raw:" + method);
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
                    expect(false, "A1.2 model socket transcript connection must not fail");
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
            return {"model/list", "modelProvider/capabilities/read"};
        }

        template <typename Result>
        std::function<void(const typed::OperationResult<Result>&)>
        handler(std::string method, codex::Json expectedResult, Phase phase) {
            return [this, method = std::move(method), expectedResult = std::move(expectedResult), phase](
                       const typed::OperationResult<Result>& operation) {
                expect(!insideSubmission, method + " completion remains asynchronous");
                if (phase == Phase::LocalRejection) {
                    ++unexpectedLocalCallbacks;
                    expect(false, method + " synchronous local rejection invokes no callback");
                    return;
                }
                if (phase == Phase::Success || phase == Phase::Generation) {
                    const bool typedRaw =
                        operation.value && operation.value->raw == expectedResult;
                    expect(operation.kind == typed::OperationResult<Result>::Kind::Success &&
                               operation.value && operation.raw == expectedResult && typedRaw,
                           method + " decodes its concrete result and retains exact raw JSON");
                    auto& order = phase == Phase::Success ? successOrder : generationOrder;
                    auto& count = phase == Phase::Success ? successCallbacks : generationCallbacks;
                    order.push_back(method);
                    ++count;
                    if (count == OperationCount) {
                        core::EventReceiver::atNextTick([this, phase]() {
                            if (phase == Phase::Success) {
                                completeSuccess();
                            } else {
                                completeGeneration();
                            }
                        });
                    }
                    if (phase == Phase::Success && method == "model/list") {
                        throw std::runtime_error("intentional model completion callback failure");
                    }
                    return;
                }
                if (phase == Phase::RemoteError) {
                    const codex::Json expectedError{
                        {"code", -32'413},
                        {"message", "synthetic model remote failure"},
                        {"data", {{"operation", method}}},
                        {"futureErrorField", true},
                    };
                    expect(operation.kind == typed::OperationResult<Result>::Kind::RemoteError &&
                               !operation.value && operation.remoteError &&
                               operation.remoteError->code == -32'413 &&
                               operation.remoteError->message == "synthetic model remote failure" &&
                               operation.remoteError->raw == expectedError && operation.raw.is_null(),
                           method + " retains the exact remote error object");
                    remoteOrder.push_back(method);
                    if (++remoteCallbacks == OperationCount) {
                        core::EventReceiver::atNextTick([this]() {
                            beginCancellation();
                        });
                    }
                    return;
                }

                expect(operation.kind == typed::OperationResult<Result>::Kind::Cancelled &&
                           !operation.value && operation.localError &&
                           operation.localError->category == codex::Error::Category::Cancelled,
                       method + " receives one cancellation at the shared connection boundary");
                cancellationOrder.push_back(method);
                ++cancellationCallbacks;
                maybeRestart();
            };
        }

        template <typename Result, typename Params, typename Submit>
        OperationCase makeOperation(std::string method,
                                    detail::ClientRequestTarget target,
                                    Params params,
                                    codex::Json expectedParams,
                                    codex::Json expectedResult,
                                    Submit submit) {
            state->successResults.emplace(method, expectedResult);
            return {
                method,
                target,
                std::move(expectedParams),
                expectedResult,
                [this, method, expectedResult, params = std::move(params), submit = std::move(submit)](Phase phase) mutable {
                    return submit(params, handler<Result>(method, expectedResult, phase));
                },
            };
        }

        void buildCases() {
            auto& models = client->typed().models();
            cases.push_back(makeOperation<typed::ModelListResponse>(
                "model/list",
                detail::ClientRequestTarget::ModelList,
                typed::ModelListParams{
                    .cursor = typed::OptionalNullable<std::string>::explicitNull(),
                    .includeHidden = typed::OptionalNullable<bool>::withValue(false),
                    .limit = typed::OptionalNullable<std::uint32_t>::withValue(25),
                },
                {{"cursor", nullptr}, {"includeHidden", false}, {"limit", 25}},
                modelResult(),
                [&models](auto params, auto resultHandler) {
                    return models.list(std::move(params), std::move(resultHandler));
                }));
            cases.push_back(makeOperation<typed::ModelProviderCapabilitiesReadResponse>(
                "modelProvider/capabilities/read",
                detail::ClientRequestTarget::ModelProviderCapabilitiesRead,
                typed::ModelProviderCapabilitiesReadParams{},
                codex::Json::object(),
                {{"imageGeneration", true},
                 {"namespaceTools", false},
                 {"webSearch", true},
                 {"futureResult", true}},
                [&models](auto params, auto resultHandler) {
                    return models.readProviderCapabilities(std::move(params), std::move(resultHandler));
                }));
        }

        void invokeLocalRejections() {
            std::size_t exact = 0;
            for (OperationCase& operation : cases) {
                insideSubmission = true;
                const Submission submission = operation.invoke(Phase::LocalRejection);
                insideSubmission = false;
                const bool registryExact = detail::entryFor(operation.target).key.name == operation.method;
                const bool rejected =
                    !submission && !submission.id && submission.error &&
                    submission.error->category == codex::Error::Category::InvalidState;
                exact += rejected && registryExact ? 1U : 0U;
                expect(rejected, operation.method + " rejects synchronously before RawProtocol is ready");
                expect(registryExact, operation.method + " dispatch starts from its exact canonical registry row");
            }
            expect(exact == OperationCount && unexpectedLocalCallbacks == 0,
                   "both model operations reject locally without callbacks or transport traffic");
        }

        void invokeBatch(Phase phase) {
            const std::size_t before = state->outbound.size();
            std::vector<std::optional<codex::ClientRequestId>> ids;
            for (OperationCase& operation : cases) {
                insideSubmission = true;
                const Submission submission = operation.invoke(phase);
                insideSubmission = false;
                expect(static_cast<bool>(submission) && submission.id && !submission.error,
                       operation.method + " is accepted by the one RawProtocol engine");
                ids.push_back(submission.id);
            }
            expect(state->outbound.size() == before + OperationCount,
                   "both model operations cross the AF_UNIX transport exactly once");
            for (std::size_t index = 0; index < cases.size(); ++index) {
                const codex::Json expectedEnvelope{
                    {"id", ids[index] ? codex::Json(ids[index]->value()) : codex::Json(nullptr)},
                    {"method", cases[index].method},
                    {"params", cases[index].expectedParams},
                };
                const OutboundRecord& record = state->outbound[before + index];
                expect(record.envelope == expectedEnvelope && record.line == expectedEnvelope.dump() + "\n",
                       cases[index].method + " sends the exact method/params JSONL bytes");
            }
            if (phase == Phase::Generation) {
                generationIds = std::move(ids);
            }
        }

        void beginSuccess() {
            state->replyMode = UnixTransportState::ReplyMode::Success;
            state->injectObservers = true;
            invokeBatch(Phase::Success);
        }

        void completeSuccess() {
            const std::vector<std::string> methods = expectedMethods();
            expect(successOrder == methods && successCallbacks == OperationCount,
                   "both model success callbacks complete asynchronously in submission order");
            std::vector<std::string> expectedObserverOrder;
            for (const std::string& method : methods) {
                expectedObserverOrder.push_back("typed:" + method);
                expectedObserverOrder.push_back("raw:" + method);
                expect(typedObserverCounts[method] == 1 && rawObserverCounts[method] == 1,
                       method + " coexists with one typed and one raw observer");
            }
            expect(observerOrder == expectedObserverOrder,
                   "typed observers precede raw observers and callback exceptions are contained");
            state->injectObservers = false;
            state->replyMode = UnixTransportState::ReplyMode::RemoteError;
            invokeBatch(Phase::RemoteError);
        }

        void beginCancellation() {
            expect(remoteOrder == expectedMethods(),
                   "both model remote errors retain callback order");
            state->replyMode = UnixTransportState::ReplyMode::Hold;
            invokeBatch(Phase::Cancellation);
            client->stop();
        }

        void maybeRestart() {
            if (!restartScheduled && firstStopObserved && cancellationCallbacks == OperationCount) {
                restartScheduled = true;
                expect(cancellationOrder == expectedMethods(),
                       "both held model operations cancel exactly once in submission order");
                core::EventReceiver::atNextTick([this]() {
                    client->start();
                });
            }
        }

        void beginGeneration() {
            state->replyMode = UnixTransportState::ReplyMode::Hold;
            invokeBatch(Phase::Generation);
            expect(state->callbackGenerations.size() >= 2,
                   "model restart installs a distinct shared transport callback generation");
            if (state->callbackGenerations.size() < 2) {
                client->stop();
                return;
            }

            const detail::TransportCallbacks stale = state->callbackGenerations.front();
            for (std::size_t index = 0; index < cases.size(); ++index) {
                if (generationIds[index]) {
                    expect(state->injectWith(
                               stale,
                               {{"id", generationIds[index]->value()}, {"result", cases[index].expectedResult}}),
                           cases[index].method + " stale response bytes cross the socket");
                }
            }
            core::EventReceiver::atNextTick([this]() {
                expect(generationCallbacks == 0,
                       "stale-generation model responses are rejected before current delivery");
                const detail::TransportCallbacks current = state->callbackGenerations.back();
                for (std::size_t index = 0; index < cases.size(); ++index) {
                    if (generationIds[index]) {
                        expect(state->injectWith(
                                   current,
                                   {{"id", generationIds[index]->value()}, {"result", cases[index].expectedResult}}),
                               cases[index].method + " current response bytes cross the socket");
                    }
                }
            });
        }

        void completeGeneration() {
            expect(generationCallbacks == OperationCount && generationOrder == expectedMethods(),
                   "both current-generation model responses complete once in callback order");
            client->stop();
        }

        tests::support::TestResult& result;
        std::shared_ptr<UnixTransportState> state;
        bool socketReady = false;
        std::unique_ptr<TestClient> client;
        std::vector<OperationCase> cases;
        std::map<std::string, std::size_t> typedObserverCounts;
        std::map<std::string, std::size_t> rawObserverCounts;
        std::vector<std::string> observerOrder;
        std::vector<std::string> successOrder;
        std::vector<std::string> remoteOrder;
        std::vector<std::string> cancellationOrder;
        std::vector<std::string> generationOrder;
        std::vector<std::optional<codex::ClientRequestId>> generationIds;
        std::size_t successCallbacks = 0;
        std::size_t remoteCallbacks = 0;
        std::size_t cancellationCallbacks = 0;
        std::size_t generationCallbacks = 0;
        std::size_t unexpectedLocalCallbacks = 0;
        int readyCount = 0;
        bool insideSubmission = false;
        bool firstStopObserved = false;
        bool restartScheduled = false;
        bool finished = false;
    };
} // namespace

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    core::SNodeC::init(argc, argv);

    ModelWireRunner runner(result);
    runner.start();

    bool timedOut = false;
    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&]() {
            timedOut = true;
            core::SNodeC::stop();
        },
        utils::Timeval({12, 0}));
    const int startResult = core::SNodeC::start(utils::Timeval({13, 0}));

    result.expectTrue(!timedOut && runner.isFinished(),
                      "A1.2 model AF_UNIX lifecycle matrix completes before the watchdog");
    result.expectEqual(0, startResult, "A1.2 model AF_UNIX lifecycle matrix stops the event loop cleanly");
    core::SNodeC::free();
    return result.processResult();
}
