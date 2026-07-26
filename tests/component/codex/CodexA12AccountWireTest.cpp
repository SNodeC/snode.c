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

    constexpr std::size_t OperationCount = 9;
    constexpr const char* SyntheticApiKey = "synthetic-api-key-for-account-wire";

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
            {"codexHome", "/tmp/codex-a1-2-account-wire"},
            {"platformFamily", "unix"},
            {"platformOs", "linux"},
            {"userAgent", "codex-a1-2-account-wire/1"},
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

            codex::Json envelope;
            try {
                envelope = codex::Json::parse(received->begin(), received->end() - 1);
            } catch (...) {
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
                                {"method", "account/updated"},
                                {"params", {{"authMode", "chatgpt"}, {"planType", "plus"}, {"operation", operation}}},
                                {"futureEnvelopeOnly", operation}})) {
                return false;
            }

            if (replyMode == ReplyMode::RemoteError) {
                return injectCurrent(
                    {{"id", envelope.at("id")},
                     {"error",
                      {{"code", -32'412},
                       {"message", "synthetic account remote failure"},
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
                              {"codex_a1_2_account_wire_test", "Codex A1.2 Account Wire Test", "1"}) {
        }
    };

    enum class Phase { LocalRejection, Success, MalformedUnit, RemoteError, Cancellation, Generation };

    struct OperationCase {
        std::string method;
        detail::ClientRequestTarget target;
        codex::Json expectedParams;
        codex::Json expectedResult;
        std::function<Submission(Phase)> invoke;
    };

    class AccountWireRunner {
    public:
        explicit AccountWireRunner(tests::support::TestResult& result)
            : result(result)
            , state(std::make_shared<UnixTransportState>())
            , socketReady(state->open())
            , client(std::make_unique<TestClient>(state)) {
            buildCases();
        }

        void start() {
            expect(socketReady, "A1.2 account wire test opens its AF_UNIX socketpair");
            if (!socketReady) {
                finished = true;
                core::SNodeC::stop();
                return;
            }

            client->typed().events().setOnEvent([this](const typed::Event& event) {
                const auto* updated = std::get_if<typed::AccountUpdatedNotification>(&event);
                if (!updated || !updated->raw.contains("params")) {
                    return;
                }
                const codex::Json& params = updated->raw.at("params");
                if (!params.contains("operation") || !params.at("operation").is_string()) {
                    return;
                }
                const std::string method = params.at("operation");
                ++typedObserverCounts[method];
                observerOrder.push_back("typed:" + method);
            });
            client->raw().setOnNotification([this](const codex::Notification& notification) {
                if (notification.method != "account/updated" || !notification.params.is_object() ||
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
                    expect(false, "A1.2 account socket transcript connection must not fail");
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
            } else if constexpr (std::is_same_v<Result, typed::LoginAccountResponse>) {
                return typed::loginAccountResponseRaw(*operation.value) == expected;
            } else {
                return operation.value->raw == expected;
            }
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
                    expect(operation.kind == typed::OperationResult<Result>::Kind::Success &&
                               operation.value && operation.raw == expectedResult &&
                               typedRawMatches(operation, expectedResult),
                           method + " decodes its exact result type and retains exact raw JSON");
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
                    if (phase == Phase::Success && method == "account/login/cancel") {
                        throw std::runtime_error("intentional account completion callback failure");
                    }
                    return;
                }
                if (phase == Phase::MalformedUnit) {
                    if constexpr (std::is_same_v<Result, typed::Unit>) {
                        expect(operation.kind == typed::OperationResult<Result>::Kind::LocalError &&
                                   !operation.value && operation.localError &&
                                   operation.localError->category == codex::Error::Category::Protocol &&
                                   operation.localError->code == EINVAL &&
                                   operation.localError->message == "Unit successful result must be the exact empty object" &&
                                   operation.raw == codex::Json{{"unexpected", true}},
                               "account/logout rejects a non-empty Unit result with the exact invariant");
                        core::EventReceiver::atNextTick([this]() {
                            beginRemoteErrors();
                        });
                    } else {
                        expect(false, "only account/logout can enter the Unit mutation phase");
                    }
                    return;
                }
                if (phase == Phase::RemoteError) {
                    const codex::Json expectedError{
                        {"code", -32'412},
                        {"message", "synthetic account remote failure"},
                        {"data", {{"operation", method}}},
                        {"futureErrorField", true},
                    };
                    expect(operation.kind == typed::OperationResult<Result>::Kind::RemoteError &&
                               !operation.value && operation.remoteError &&
                               operation.remoteError->code == -32'412 &&
                               operation.remoteError->message == "synthetic account remote failure" &&
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
            auto& accounts = client->typed().accounts();
            cases.push_back(makeOperation<typed::CancelLoginAccountResponse>(
                "account/login/cancel",
                detail::ClientRequestTarget::AccountLoginCancel,
                typed::CancelLoginAccountParams{typed::LoginId{"login-wire"}},
                {{"loginId", "login-wire"}},
                {{"status", "canceled"}},
                [&accounts](auto params, auto resultHandler) {
                    return accounts.cancelLogin(std::move(params), std::move(resultHandler));
                }));
            cases.push_back(makeOperation<typed::LoginAccountResponse>(
                "account/login/start",
                detail::ClientRequestTarget::AccountLoginStart,
                typed::LoginAccountParams{typed::ApiKeyLoginAccountParams{SyntheticApiKey}},
                {{"apiKey", SyntheticApiKey}, {"type", "apiKey"}},
                {{"type", "apiKey"}},
                [&accounts](auto params, auto resultHandler) {
                    return accounts.startLogin(std::move(params), std::move(resultHandler));
                }));
            cases.push_back(makeOperation<typed::Unit>(
                "account/logout",
                detail::ClientRequestTarget::AccountLogout,
                typed::Unit{},
                nullptr,
                codex::Json::object(),
                [&accounts](auto params, auto resultHandler) {
                    return accounts.logout(std::move(params), std::move(resultHandler));
                }));
            cases.push_back(makeOperation<typed::ConsumeAccountRateLimitResetCreditResponse>(
                "account/rateLimitResetCredit/consume",
                detail::ClientRequestTarget::AccountRateLimitResetCreditConsume,
                typed::ConsumeAccountRateLimitResetCreditParams{
                    typed::OptionalNullable<typed::RateLimitResetCreditId>::explicitNull(),
                    typed::IdempotencyKey{"idempotency-wire"},
                },
                {{"creditId", nullptr}, {"idempotencyKey", "idempotency-wire"}},
                {{"outcome", "reset"}},
                [&accounts](auto params, auto resultHandler) {
                    return accounts.consumeRateLimitResetCredit(std::move(params), std::move(resultHandler));
                }));
            cases.push_back(makeOperation<typed::GetAccountRateLimitsResponse>(
                "account/rateLimits/read",
                detail::ClientRequestTarget::AccountRateLimitsRead,
                typed::Unit{},
                nullptr,
                {{"rateLimits", codex::Json::object()}},
                [&accounts](auto params, auto resultHandler) {
                    return accounts.readRateLimits(std::move(params), std::move(resultHandler));
                }));
            cases.push_back(makeOperation<typed::GetAccountResponse>(
                "account/read",
                detail::ClientRequestTarget::AccountRead,
                typed::GetAccountParams{false},
                {{"refreshToken", false}},
                {{"account", {{"type", "apiKey"}}}, {"requiresOpenaiAuth", false}},
                [&accounts](auto params, auto resultHandler) {
                    return accounts.read(std::move(params), std::move(resultHandler));
                }));
            cases.push_back(makeOperation<typed::SendAddCreditsNudgeEmailResponse>(
                "account/sendAddCreditsNudgeEmail",
                detail::ClientRequestTarget::AccountSendAddCreditsNudgeEmail,
                typed::SendAddCreditsNudgeEmailParams{typed::AddCreditsNudgeCreditType::credits()},
                {{"creditType", "credits"}},
                {{"status", "sent"}},
                [&accounts](auto params, auto resultHandler) {
                    return accounts.sendAddCreditsNudgeEmail(std::move(params), std::move(resultHandler));
                }));
            cases.push_back(makeOperation<typed::GetAccountTokenUsageResponse>(
                "account/usage/read",
                detail::ClientRequestTarget::AccountUsageRead,
                typed::Unit{},
                nullptr,
                {{"summary", codex::Json::object()}},
                [&accounts](auto params, auto resultHandler) {
                    return accounts.readUsage(std::move(params), std::move(resultHandler));
                }));
            cases.push_back(makeOperation<typed::GetWorkspaceMessagesResponse>(
                "account/workspaceMessages/read",
                detail::ClientRequestTarget::AccountWorkspaceMessagesRead,
                typed::Unit{},
                nullptr,
                {{"featureEnabled", true}, {"messages", codex::Json::array()}},
                [&accounts](auto params, auto resultHandler) {
                    return accounts.readWorkspaceMessages(std::move(params), std::move(resultHandler));
                }));
        }

        void invokeLocalRejections() {
            std::size_t exact = 0;
            for (OperationCase& operation : cases) {
                insideSubmission = true;
                const Submission submission = operation.invoke(Phase::LocalRejection);
                insideSubmission = false;
                const bool registryExact =
                    detail::entryFor(operation.target).key.name == operation.method;
                const bool rejected =
                    !submission && !submission.id && submission.error &&
                    submission.error->category == codex::Error::Category::InvalidState;
                exact += rejected && registryExact ? 1U : 0U;
                expect(rejected, operation.method + " rejects synchronously before the shared protocol is ready");
                expect(registryExact, operation.method + " dispatch starts from its exact canonical registry row");
            }
            expect(exact == OperationCount && unexpectedLocalCallbacks == 0,
                   "all nine account methods reject locally without callbacks or transport traffic");

            const Submission unknown = client->typed().accounts().startLogin(
                typed::UnknownLoginAccountParams{std::string("future-login")},
                [this](const typed::OperationResult<typed::LoginAccountResponse>&) {
                    ++unexpectedLocalCallbacks;
                });
            expect(!unknown && unknown.error && unknown.error->category == codex::Error::Category::Protocol &&
                       unknown.error->message == "LoginAccountParams future discriminator cannot be encoded",
                   "future outgoing login union rejects synchronously before reaching RawProtocol");
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
            expect(state->outbound.size() == before + cases.size(),
                   "all nine account operations cross the AF_UNIX transport exactly once");
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
                   "all nine account success callbacks complete asynchronously in submission order");
            expect(unexpectedLocalCallbacks == 0,
                   "synchronous rejection handlers remain uncalled after successful traffic");

            std::vector<std::string> expectedObserverOrder;
            for (const std::string& method : methods) {
                expectedObserverOrder.push_back("typed:" + method);
                expectedObserverOrder.push_back("raw:" + method);
                expect(typedObserverCounts[method] == 1 && rawObserverCounts[method] == 1,
                       method + " coexists with one typed and one raw notification observation");
            }
            expect(observerOrder == expectedObserverOrder,
                   "typed observers precede coexisting raw observers and callback exceptions are contained");

            state->injectObservers = false;
            state->successResults["account/logout"] = {{"unexpected", true}};
            OperationCase& logout = cases[2];
            insideSubmission = true;
            const Submission submission = logout.invoke(Phase::MalformedUnit);
            insideSubmission = false;
            expect(static_cast<bool>(submission), "account/logout Unit mutation request is accepted");
        }

        void beginRemoteErrors() {
            state->successResults["account/logout"] = codex::Json::object();
            state->replyMode = UnixTransportState::ReplyMode::RemoteError;
            invokeBatch(Phase::RemoteError);
        }

        void beginCancellation() {
            expect(remoteCallbacks == OperationCount && remoteOrder == expectedMethods(),
                   "all nine account remote errors retain callback order");
            state->replyMode = UnixTransportState::ReplyMode::Hold;
            invokeBatch(Phase::Cancellation);
            client->stop();
        }

        void maybeRestart() {
            if (!restartScheduled && firstStopObserved && cancellationCallbacks == OperationCount) {
                restartScheduled = true;
                expect(cancellationOrder == expectedMethods(),
                       "all nine held account operations cancel exactly once in submission order");
                core::EventReceiver::atNextTick([this]() {
                    client->start();
                });
            }
        }

        void beginGeneration() {
            state->replyMode = UnixTransportState::ReplyMode::Hold;
            invokeBatch(Phase::Generation);
            expect(state->callbackGenerations.size() >= 2,
                   "account restart installs a distinct shared transport callback generation");
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
                       "all stale-generation account responses are rejected before current delivery");
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
                   "all nine current-generation account responses complete once in callback order");
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

    AccountWireRunner runner(result);
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
                      "A1.2 account AF_UNIX lifecycle matrix completes before the watchdog");
    result.expectEqual(0, startResult, "A1.2 account AF_UNIX lifecycle matrix stops the event loop cleanly");
    core::SNodeC::free();
    return result.processResult();
}
