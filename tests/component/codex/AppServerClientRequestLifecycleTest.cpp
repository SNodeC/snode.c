#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "core/timer/Timer.h"
#include "support/SemanticLogCapture.h"
#include "support/TestResult.h"
#include "tests/component/codex/CodexBackendTestSupport.h"
#include "utils/Timeval.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
    namespace codex = ai::openai::codex;

    class LifecycleRunner {
    public:
        explicit LifecycleRunner(tests::support::TestResult& result)
            : result(result)
            , transport(std::make_shared<tests::codex::FakeTransportState>()) {
        }

        void start() {
            tests::codex::installInitializingFake(
                transport, [](const codex::Json& message, const codex::detail::TransportCallbacks& callbacks) {
                    const std::string method = message.value("method", "");
                    if (method == "request-lifecycle/success") {
                        tests::codex::inject(callbacks, {{"id", message.at("id")}, {"result", {{"ok", true}}}});
                    } else if (method == "request-lifecycle/error") {
                        tests::codex::inject(
                            callbacks,
                            {{"id", message.at("id")}, {"error", {{"code", -32003}, {"message", "deterministic remote error"}}}});
                    }
                });

            client = std::make_unique<tests::codex::FakeAppServerClient>(transport);
            client->raw().setOnNotification([this](const codex::Notification& notification) {
                if (notification.method == "request-lifecycle/incoming-notification") {
                    ++incomingNotifications;
                }
            });
            client->setOnStateChanged([this](const codex::StateChange& change) {
                stateChanged(change);
            });
            client->start();
        }

        bool isFinished() const noexcept {
            return finished;
        }

        std::optional<std::int64_t> successId;
        std::optional<std::int64_t> errorId;
        std::optional<std::int64_t> pendingId;
        int successCallbacks = 0;
        int errorCallbacks = 0;
        int cancellationCallbacks = 0;
        int incomingNotifications = 0;

    private:
        void stateChanged(const codex::StateChange& change) {
            if (change.current == codex::State::Ready && !requestsSubmitted) {
                requestsSubmitted = true;
                submitRequests();
                core::EventReceiver::atNextTick([this]() {
                    client->stop();
                    if (pendingId) {
                        transport->inject({{"id", *pendingId}, {"result", {{"late", true}}}});
                    }
                });
            } else if (change.current == codex::State::Stopped && requestsSubmitted) {
                result.expectEqual(1, successCallbacks, "successful App Server request callback runs once");
                result.expectEqual(1, errorCallbacks, "remote-error App Server request callback runs once");
                result.expectEqual(1, cancellationCallbacks, "intentional stop cancels the pending App Server request once");
                result.expectEqual(1, incomingNotifications, "incoming JSON-RPC notification is delivered once");
                core::EventReceiver::atNextTick([this]() {
                    client.reset();
                    finished = true;
                    core::SNodeC::stop();
                });
            }
        }

        void submitRequests() {
            const auto success = client->raw().request("request-lifecycle/success", codex::Json::object(), [this](const codex::Response& response) {
                ++successCallbacks;
                result.expectTrue(response.kind == codex::Response::Kind::Result && response.method == "request-lifecycle/success",
                                  "successful request callback preserves its result and method");
            });
            result.expectTrue(static_cast<bool>(success), "successful lifecycle request is accepted");
            if (success.id) {
                successId = success.id->value();
            }

            const auto remoteError = client->raw().request("request-lifecycle/error", codex::Json::object(), [this](const codex::Response& response) {
                ++errorCallbacks;
                result.expectTrue(response.kind == codex::Response::Kind::RemoteError && response.method == "request-lifecycle/error",
                                  "remote-error callback preserves its error outcome and method");
            });
            result.expectTrue(static_cast<bool>(remoteError), "remote-error lifecycle request is accepted");
            if (remoteError.id) {
                errorId = remoteError.id->value();
            }

            const auto pending = client->raw().request("request-lifecycle/pending", codex::Json::object(), [this](const codex::Response& response) {
                ++cancellationCallbacks;
                result.expectTrue(response.kind == codex::Response::Kind::Cancelled && response.method == "request-lifecycle/pending",
                                  "intentional stop preserves the existing cancellation callback semantics");
            });
            result.expectTrue(static_cast<bool>(pending), "pending lifecycle request is accepted before intentional stop");
            if (pending.id) {
                pendingId = pending.id->value();
            }

            const auto notification = client->raw().notify("request-lifecycle/outgoing-notification", {{"value", 1}});
            result.expectTrue(static_cast<bool>(notification), "outgoing JSON-RPC notification is accepted");
            transport->inject({{"method", "request-lifecycle/incoming-notification"}, {"params", {{"value", 2}}}});
        }

        tests::support::TestResult& result;
        std::shared_ptr<tests::codex::FakeTransportState> transport;
        std::unique_ptr<tests::codex::FakeAppServerClient> client;
        bool requestsSubmitted = false;
        bool finished = false;
    };

    int countMessage(const std::vector<nlohmann::json>& records, const std::string& expected) {
        int count = 0;
        for (const nlohmann::json& record : records) {
            count += record.value("message", "") == expected ? 1 : 0;
        }
        return count;
    }

    int countPrefix(const std::vector<nlohmann::json>& records, std::string_view prefix) {
        int count = 0;
        for (const nlohmann::json& record : records) {
            count += record.value("message", "").starts_with(prefix) ? 1 : 0;
        }
        return count;
    }

    bool requestScopeIsCanonical(const std::vector<nlohmann::json>& records) {
        for (const nlohmann::json& record : records) {
            if (record.value("message", "").starts_with("request ") &&
                (record.value("level", "") != "debug" || record.value("origin", "") != "framework" ||
                 record.value("boundary", "") != "connection" || record.value("component", "") != "ai.openai.codex")) {
                return false;
            }
        }
        return true;
    }

    bool notificationHasNoRequestRecord(const std::vector<nlohmann::json>& records) {
        for (const nlohmann::json& record : records) {
            const std::string message = record.value("message", "");
            if (message.starts_with("request ") && (message.find("request-lifecycle/outgoing-notification") != std::string::npos ||
                                                    message.find("request-lifecycle/incoming-notification") != std::string::npos)) {
                return false;
            }
        }
        return true;
    }
} // namespace

int main() {
    tests::support::TestResult result;
    int returnCode = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("AppServerClientRequestLifecycleTest");
    } else {
        tests::support::SemanticLogCapture capture("snodec-app-server-client-request-lifecycle");
        capture.initCore("AppServerClientRequestLifecycleTest");

        bool timedOut = false;
        LifecycleRunner runner(result);
        [[maybe_unused]] core::timer::Timer watchdog = core::timer::Timer::singleshotTimer(
            [&timedOut]() {
                timedOut = true;
                core::SNodeC::stop();
            },
            utils::Timeval({5, 0}));

        runner.start();
        const int eventLoopResult = core::SNodeC::start(utils::Timeval({7, 0}));
        result.expectTrue(!timedOut, "App Server lifecycle fake-transport scenario completes before its watchdog");
        result.expectTrue(runner.isFinished(), "App Server lifecycle fake-transport scenario reaches intentional stop");
        result.expectEqual(0, eventLoopResult, "App Server lifecycle event loop stops cleanly");
        core::SNodeC::free();

        const std::vector<nlohmann::json> records = capture.finish();
        result.expectTrue(runner.successId && runner.errorId && runner.pendingId, "all accepted requests retain their JSON-RPC IDs");
        if (runner.successId && runner.errorId && runner.pendingId) {
            const std::string successIdentity = "id=" + std::to_string(*runner.successId) + " method=request-lifecycle/success";
            const std::string errorIdentity = "id=" + std::to_string(*runner.errorId) + " method=request-lifecycle/error";
            const std::string pendingIdentity = "id=" + std::to_string(*runner.pendingId) + " method=request-lifecycle/pending";

            result.expectEqual(1, countMessage(records, "request started: " + successIdentity), "successful request starts once");
            result.expectEqual(1, countMessage(records, "request completed: " + successIdentity), "successful request completes once");
            result.expectEqual(0, countMessage(records, "request failed: " + successIdentity), "successful request has no failure");
            result.expectEqual(1, countMessage(records, "request started: " + errorIdentity), "remote-error request starts once");
            result.expectEqual(1, countMessage(records, "request failed: " + errorIdentity), "remote-error request fails once");
            result.expectEqual(0, countMessage(records, "request completed: " + errorIdentity), "remote-error request has no completion");
            result.expectEqual(1, countMessage(records, "request started: " + pendingIdentity), "pending request starts once");
            result.expectEqual(
                1, countMessage(records, "request cancelled: " + pendingIdentity), "intentional stop cancels pending request once");
            result.expectEqual(
                0, countMessage(records, "request completed: " + pendingIdentity), "late response cannot complete cancelled request");
            result.expectEqual(0, countMessage(records, "request failed: " + pendingIdentity), "intentional stop is not a request failure");
        }

        result.expectEqual(3, countPrefix(records, "request started:"), "only the three JSON-RPC requests receive starts");
        result.expectEqual(1, countPrefix(records, "request completed:"), "one request completes");
        result.expectEqual(1, countPrefix(records, "request failed:"), "one remote request fails");
        result.expectEqual(1, countPrefix(records, "request cancelled:"), "one pending request is cancelled");
        result.expectTrue(notificationHasNoRequestRecord(records), "JSON-RPC notifications create no request lifecycle pair");
        result.expectTrue(requestScopeIsCanonical(records), "request lifecycle records share the canonical Debug semantic scope");
        returnCode = result.processResult();
    }

    return returnCode;
}
