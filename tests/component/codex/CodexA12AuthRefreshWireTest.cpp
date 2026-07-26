/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/detail/Transport.h"
#include "ai/openai/codex/typed/Client.h"
#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/SemanticLogCapture.h"
#include "support/TestResult.h"
#include "utils/Timeval.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <memory>
#include <optional>
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

    constexpr const char* SyntheticAccessToken = "synthetic-auth-refresh-token";
    constexpr const char* SyntheticAccountId = "synthetic-auth-refresh-account";

    bool textContainsSyntheticCredential(std::string_view value) {
        return value.find(SyntheticAccessToken) != std::string_view::npos ||
               value.find(SyntheticAccountId) != std::string_view::npos;
    }

    bool containsSyntheticCredential(const codex::Json& value) {
        if (value.is_string()) {
            return textContainsSyntheticCredential(
                value.get_ref<const std::string&>());
        }
        if (value.is_array() || value.is_object()) {
            return std::any_of(
                value.begin(), value.end(), [](const codex::Json& nested) {
                    return containsSyntheticCredential(nested);
                });
        }
        return false;
    }

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
            {"codexHome", "/tmp/codex-a1-2-auth-wire"},
            {"platformFamily", "unix"},
            {"platformOs", "linux"},
            {"userAgent", "codex-a1-2-auth-wire/1"},
        };
    }

    struct WireRecord {
        std::string line;
        codex::Json envelope;
    };

    struct UnixTransportState {
        detail::TransportCallbacks callbacks;
        std::vector<WireRecord> outgoing;
        std::vector<WireRecord> incoming;
        int clientDescriptor = -1;
        int serverDescriptor = -1;
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

        bool inject(const codex::Json& envelope) {
            const std::string serverLine = envelope.dump() + "\n";
            if (!writeFully(serverDescriptor, serverLine)) {
                return false;
            }
            const std::optional<std::string> received = readLine(clientDescriptor);
            if (!received || *received != serverLine || !callbacks.onMessage) {
                return false;
            }
            incoming.push_back({*received, envelope});
            callbacks.onMessage(received->substr(0, received->size() - 1));
            return true;
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
            outgoing.push_back({*received, envelope});

            const auto method = envelope.find("method");
            if (method != envelope.end() && method->is_string() && *method == "initialize") {
                const auto id = envelope.find("id");
                return id != envelope.end() && inject({{"id", *id}, {"result", initializeResult()}});
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
                              {"codex_a1_2_auth_wire_test", "Codex A1.2 Auth Wire Test", "1"}) {
        }
    };

    class AuthRefreshRunner {
    public:
        explicit AuthRefreshRunner(tests::support::TestResult& result)
            : result(result)
            , state(std::make_shared<UnixTransportState>())
            , socketReady(state->open())
            , client(std::make_unique<TestClient>(state)) {
        }

        void start() {
            expect(socketReady, "auth-refresh wire test opens its AF_UNIX socketpair");
            if (!socketReady) {
                finished = true;
                core::SNodeC::stop();
                return;
            }

            client->typed().requests().setOnRequest([this](const typed::TypedServerRequest& request) {
                typedObserverOrder.push_back(requestId(request));
                const auto* authentication = std::get_if<typed::AuthenticationRequest>(&request);
                expect(authentication != nullptr,
                       "account/chatgptAuthTokens/refresh reaches the schema-complete typed request alternative");
                if (authentication) {
                    handleAuthentication(*authentication);
                }
            });
            client->raw().setOnServerRequest([this](const codex::ServerRequest& request) {
                rawObserverOrder.push_back(requestId(request.id));
                if (request.id == codex::ServerRequestId{std::string("auth-shared")} && rawObserverOrder.size() == 1) {
                    core::EventReceiver::atNextTick([this]() {
                        injectSecondOccurrence();
                    });
                } else if (request.id == codex::ServerRequestId{std::string("auth-shared")}) {
                    core::EventReceiver::atNextTick([this]() {
                        injectFirstGenerationPending();
                    });
                } else if (request.id == codex::ServerRequestId{std::string("auth-generation")} && readyCount == 1) {
                    core::EventReceiver::atNextTick([this]() {
                        client->stop();
                    });
                } else if (request.id == codex::ServerRequestId{std::string("auth-generation")} && readyCount == 2) {
                    core::EventReceiver::atNextTick([this]() {
                        finishAfterFreshGeneration();
                    });
                }
            });

            client->setOnStateChanged([this](const codex::StateChange& change) {
                if (change.current == codex::State::Ready) {
                    ++readyCount;
                    if (readyCount == 1) {
                        injectFirstOccurrence();
                    } else if (readyCount == 2) {
                        injectFreshGenerationOccurrence();
                    }
                } else if (change.current == codex::State::Failed) {
                    expect(false, "auth-refresh socket transcript connection must not fail");
                    client->stop();
                } else if (change.current == codex::State::Stopped) {
                    if (readyCount == 1) {
                        core::EventReceiver::atNextTick([this]() {
                            client->start();
                        });
                    } else if (readyCount == 2) {
                        finished = true;
                        core::SNodeC::stop();
                    }
                }
            });
            client->start();
        }

        bool isFinished() const noexcept {
            return finished;
        }

        bool hasSecretFreeErrorsAndDiagnostics() const noexcept {
            return secretFreeErrorsAndDiagnostics;
        }

    private:
        void expect(bool condition, const std::string& description) {
            result.expectTrue(condition, description);
        }

        static std::string requestId(const codex::ServerRequestId& id) {
            return std::visit(
                [](const auto& value) -> std::string {
                    using Value = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<Value, std::string>) {
                        return value;
                    } else {
                        return std::to_string(value);
                    }
                },
                id.value());
        }

        static std::string requestId(const typed::TypedServerRequest& request) {
            return std::visit([](const auto& value) { return requestId(value.requestId); }, request);
        }

        void inspectSensitiveText(std::string_view text) {
            secretFreeErrorsAndDiagnostics =
                secretFreeErrorsAndDiagnostics &&
                !textContainsSyntheticCredential(text);
        }

        void inspectDiagnostics(
            const std::vector<typed::DecodeDiagnostic>& diagnostics) {
            for (const typed::DecodeDiagnostic& diagnostic : diagnostics) {
                inspectSensitiveText(diagnostic.surface);
                inspectSensitiveText(diagnostic.fieldPath);
                inspectSensitiveText(diagnostic.message);
            }
        }

        codex::Json requestEnvelope(std::string id, codex::Json params, std::string futureEnvelope) {
            return {
                {"jsonrpc", "2.0"},
                {"id", std::move(id)},
                {"method", "account/chatgptAuthTokens/refresh"},
                {"params", std::move(params)},
                {"futureEnvelopeOnly", std::move(futureEnvelope)},
            };
        }

        bool injectExact(const codex::Json& envelope, const std::string& label) {
            const std::size_t before = state->incoming.size();
            const bool accepted = state->inject(envelope);
            const bool exact =
                accepted && state->incoming.size() == before + 1 &&
                state->incoming.back().envelope == envelope &&
                state->incoming.back().line == envelope.dump() + "\n";
            expect(exact, label + " crosses AF_UNIX as exact incoming JSONL bytes");
            return exact;
        }

        void injectFirstOccurrence() {
            firstEnvelope = requestEnvelope(
                "auth-shared",
                {{"previousAccountId", SyntheticAccountId}, {"reason", "unauthorized"}},
                "first");
            injectExact(firstEnvelope, "first auth-refresh occurrence");
        }

        void injectSecondOccurrence() {
            secondEnvelope = requestEnvelope(
                "auth-shared",
                {{"previousAccountId", nullptr}, {"reason", "future-reason"}},
                "second");
            injectExact(secondEnvelope, "second same-ID auth-refresh occurrence");
        }

        void injectFirstGenerationPending() {
            firstGenerationEnvelope =
                requestEnvelope("auth-generation", {{"reason", "unauthorized"}}, "generation-one");
            injectExact(firstGenerationEnvelope, "first-generation unanswered auth-refresh occurrence");
        }

        void injectFreshGenerationOccurrence() {
            freshGenerationEnvelope = requestEnvelope(
                "auth-generation",
                {{"previousAccountId", "fresh-account"}, {"reason", "unauthorized"}},
                "generation-two");
            injectExact(freshGenerationEnvelope, "fresh same-ID auth-refresh occurrence");
        }

        std::size_t responseCount(std::string_view id) const {
            std::size_t count = 0;
            for (const WireRecord& record : state->outgoing) {
                const auto responseId = record.envelope.find("id");
                if (!record.envelope.contains("method") && responseId != record.envelope.end() &&
                    responseId->is_string() && *responseId == id) {
                    ++count;
                }
            }
            return count;
        }

        bool hasExactResponse(std::string_view id, const codex::Json& response) const {
            for (const WireRecord& record : state->outgoing) {
                const auto responseId = record.envelope.find("id");
                if (!record.envelope.contains("method") && responseId != record.envelope.end() &&
                    responseId->is_string() && *responseId == id &&
                    record.envelope == response && record.line == response.dump() + "\n") {
                    return true;
                }
            }
            return false;
        }

        void handleAuthentication(const typed::AuthenticationRequest& request) {
            const std::string id = requestId(request.requestId);
            if (id == "auth-shared" && !firstRequest) {
                firstRequest = request;
                expect(request.raw == firstEnvelope &&
                           request.canonicalParams.raw == firstEnvelope.at("params") &&
                           request.canonicalParams.previousAccountId.hasValue() &&
                           request.canonicalParams.previousAccountId->value == SyntheticAccountId &&
                           request.canonicalParams.reason == typed::ChatgptAuthTokensRefreshReason::unauthorized() &&
                           request.reason == "unauthorized" && request.previousAccountId == SyntheticAccountId,
                       "first auth-refresh request preserves full envelope, exact params, strong identity, and compatibility view");

                const auto sent = client->typed().requests().respondRefresh(
                    request,
                    typed::ChatgptAuthTokensRefreshResponse{
                        SyntheticAccessToken,
                        typed::AccountId{SyntheticAccountId},
                        typed::PlanType::plus(),
                    });
                expect(static_cast<bool>(sent), "canonical auth-refresh response is accepted exactly once");
                const std::size_t beforeDuplicate = state->outgoing.size();
                const auto duplicate = client->typed().requests().respondRefresh(
                    request,
                    typed::ChatgptAuthTokensRefreshResponse{
                        SyntheticAccessToken,
                        typed::AccountId{SyntheticAccountId},
                        typed::PlanType::plus(),
                    });
                if (duplicate.error) {
                    inspectSensitiveText(duplicate.error->message);
                }
                expect(!duplicate && duplicate.error &&
                           duplicate.error->category == codex::Error::Category::InvalidState &&
                           duplicate.error->message.find(SyntheticAccessToken) == std::string::npos &&
                           duplicate.error->message.find(SyntheticAccountId) == std::string::npos &&
                           state->outgoing.size() == beforeDuplicate,
                       "duplicate auth-refresh response is rejected without wire output or credential disclosure");
                duplicateRejected = true;
                return;
            }

            if (id == "auth-shared") {
                inspectDiagnostics(request.diagnostics);
                expect(firstRequest.has_value(), "first same-ID occurrence remains available for stale-token validation");
                expect(request.raw == secondEnvelope && request.canonicalParams.previousAccountId.isNull() &&
                           !request.previousAccountId &&
                           request.canonicalParams.reason.value == "future-reason" &&
                           !request.canonicalParams.reason.isKnown() &&
                           request.diagnostics.size() == 1 &&
                           request.diagnostics.front().kind == typed::DecodeIssueKind::UnknownEnumValue &&
                           request.diagnostics.front().severity == typed::DecodeIssueSeverity::ForwardCompatibility &&
                           request.diagnostics.front().fieldPath == "$.params.reason",
                       "second auth-refresh occurrence preserves explicit null and future reason without losing its known outer request");
                const std::size_t beforeStale = state->outgoing.size();
                if (firstRequest) {
                    const auto stale = client->typed().requests().respondRefresh(
                        *firstRequest,
                        typed::ChatgptAuthTokensRefreshResponse{
                            SyntheticAccessToken,
                            typed::AccountId{SyntheticAccountId},
                            typed::OptionalNullable<typed::PlanType>::explicitNull(),
                        });
                    if (stale.error) {
                        inspectSensitiveText(stale.error->message);
                    }
                    expect(!stale && stale.error && stale.error->category == codex::Error::Category::InvalidState &&
                               stale.error->code == ESTALE &&
                               stale.error->message.find(SyntheticAccessToken) == std::string::npos &&
                               stale.error->message.find(SyntheticAccountId) == std::string::npos &&
                               state->outgoing.size() == beforeStale,
                           "old occurrence token cannot answer a fresh same-ID auth request and leaks no secret");
                    staleSameGenerationRejected = true;
                }
                const auto legacy = client->typed().requests().respond(
                    request,
                    typed::AuthenticationResponse{
                        SyntheticAccessToken,
                        SyntheticAccountId,
                        std::nullopt,
                    });
                expect(static_cast<bool>(legacy),
                       "legacy AuthenticationResponse adapter remains source-compatible and answers the current occurrence");
                return;
            }

            if (readyCount == 1) {
                firstGenerationRequest = request;
                expect(request.raw == firstGenerationEnvelope &&
                           request.canonicalParams.previousAccountId.isOmitted() &&
                           !request.previousAccountId,
                       "first-generation pending auth request retains an omitted previous account ID");
                return;
            }

            expect(firstGenerationRequest.has_value(),
                   "disconnected auth request remains available only for explicit stale-token rejection");
            expect(request.raw == freshGenerationEnvelope &&
                       request.canonicalParams.previousAccountId.hasValue() &&
                       request.canonicalParams.previousAccountId->value == "fresh-account",
                   "fresh generation receives a distinct exact auth-refresh occurrence");
            const std::size_t beforeStale = state->outgoing.size();
            if (firstGenerationRequest) {
                const auto stale = client->typed().requests().respondRefresh(
                    *firstGenerationRequest,
                    typed::ChatgptAuthTokensRefreshResponse{
                        SyntheticAccessToken,
                        typed::AccountId{SyntheticAccountId},
                        typed::OptionalNullable<typed::PlanType>::omitted(),
                    });
                if (stale.error) {
                    inspectSensitiveText(stale.error->message);
                }
                expect(!stale && stale.error && stale.error->category == codex::Error::Category::InvalidState &&
                           stale.error->code == ESTALE &&
                           stale.error->message.find(SyntheticAccessToken) == std::string::npos &&
                           stale.error->message.find(SyntheticAccountId) == std::string::npos &&
                           state->outgoing.size() == beforeStale,
                       "disconnected occurrence token cannot answer a same-ID request in a new generation");
                staleGenerationRejected = true;
            }
            const auto fresh = client->typed().requests().respondRefresh(
                request,
                typed::ChatgptAuthTokensRefreshResponse{
                    SyntheticAccessToken,
                    typed::AccountId{SyntheticAccountId},
                    typed::OptionalNullable<typed::PlanType>::omitted(),
                });
            expect(static_cast<bool>(fresh), "fresh-generation occurrence remains answerable after stale rejection");
        }

        void finishAfterFreshGeneration() {
            const codex::Json firstResponse{
                {"id", "auth-shared"},
                {"result",
                 {{"accessToken", SyntheticAccessToken},
                  {"chatgptAccountId", SyntheticAccountId},
                  {"chatgptPlanType", "plus"}}},
            };
            const codex::Json secondResponse{
                {"id", "auth-shared"},
                {"result",
                 {{"accessToken", SyntheticAccessToken},
                  {"chatgptAccountId", SyntheticAccountId},
                  {"chatgptPlanType", nullptr}}},
            };
            const codex::Json freshResponse{
                {"id", "auth-generation"},
                {"result",
                 {{"accessToken", SyntheticAccessToken},
                  {"chatgptAccountId", SyntheticAccountId}}},
            };
            expect(responseCount("auth-shared") == 2 &&
                       hasExactResponse("auth-shared", firstResponse) &&
                       hasExactResponse("auth-shared", secondResponse),
                   "two same-ID occurrences emit exactly their canonical and compatibility JSONL response bytes");
            expect(responseCount("auth-generation") == 1 &&
                       hasExactResponse("auth-generation", freshResponse),
                   "only the fresh generation emits one exact response with an omitted plan field");
            expect(duplicateRejected && staleSameGenerationRejected && staleGenerationRejected,
                   "duplicate, stale occurrence, and stale generation response guards all execute");
            expect(typedObserverOrder == rawObserverOrder &&
                       typedObserverOrder ==
                           std::vector<std::string>({"auth-shared", "auth-shared", "auth-generation", "auth-generation"}),
                   "typed and raw server-request observers coexist once per occurrence in deterministic order");
            client->stop();
        }

        tests::support::TestResult& result;
        std::shared_ptr<UnixTransportState> state;
        bool socketReady = false;
        std::unique_ptr<TestClient> client;
        std::optional<typed::AuthenticationRequest> firstRequest;
        std::optional<typed::AuthenticationRequest> firstGenerationRequest;
        codex::Json firstEnvelope;
        codex::Json secondEnvelope;
        codex::Json firstGenerationEnvelope;
        codex::Json freshGenerationEnvelope;
        std::vector<std::string> typedObserverOrder;
        std::vector<std::string> rawObserverOrder;
        int readyCount = 0;
        bool duplicateRejected = false;
        bool staleSameGenerationRejected = false;
        bool staleGenerationRejected = false;
        bool secretFreeErrorsAndDiagnostics = true;
        bool finished = false;
    };
} // namespace

int main() {
    tests::support::TestResult result;
    tests::support::SemanticLogCapture capture(
        "snodec-codex-a1-2-auth-refresh-wire");
    capture.initCore("CodexA12AuthRefreshWireTest");

    AuthRefreshRunner runner(result);
    runner.start();

    bool timedOut = false;
    [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
        [&]() {
            timedOut = true;
            core::SNodeC::stop();
        },
        utils::Timeval({10, 0}));
    const int startResult = core::SNodeC::start(utils::Timeval({11, 0}));

    result.expectTrue(!timedOut && runner.isFinished(),
                      "A1.2 auth-refresh occurrence/generation matrix completes before the watchdog");
    result.expectEqual(0, startResult, "A1.2 auth-refresh AF_UNIX matrix stops the event loop cleanly");
    core::SNodeC::free();

    const std::vector<nlohmann::json> semanticLogRecords = capture.finish();
    const bool semanticLogsAreSecretFree = std::none_of(
        semanticLogRecords.begin(),
        semanticLogRecords.end(),
        [](const nlohmann::json& record) {
            return containsSyntheticCredential(record);
        });
    result.expectTrue(
        !semanticLogRecords.empty(),
        "auth-refresh request and response execute while the real semantic logger capture is active");
    result.expectTrue(
        semanticLogsAreSecretFree,
        "captured semantic logs contain no synthetic auth token or account identity");
    result.expectTrue(
        runner.hasSecretFreeErrorsAndDiagnostics(),
        "auth-refresh rejection errors and decode diagnostics contain no synthetic auth token or account identity");
    return result.processResult();
}
