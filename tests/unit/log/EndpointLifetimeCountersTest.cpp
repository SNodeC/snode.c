#include "core/socket/stream/ClientFlowController.h"
#include "core/socket/stream/FlowController.hpp"
#include "core/socket/stream/ServerFlowController.h"
#include "log/LogScopeOwner.h"
#include "log/Logger.h"
#include "tests/support/TestResult.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {
    struct ExposedRetryFlowController : public core::socket::stream::FlowController<ExposedRetryFlowController> {
        ExposedRetryFlowController()
            : FlowController("retry-counter", {}) {
        }
        void dispatchRetry() {
            reportFlowRetry();
        }
        void terminateAsyncSubFlow() override {
        }
    };

    struct ExposedClientFlowController : public core::socket::stream::ClientFlowController {
        explicit ExposedClientFlowController(const std::string& instanceName = "reconnect-counter")
            : ClientFlowController(instanceName, {}) {
        }
        void dispatchReconnect() {
            reportFlowReconnect();
        }
    };

    struct ServerEndpointProbe {
        explicit ServerEndpointProbe(std::string instanceName)
            : flowController(instanceName, {})
            , logScope(logger::LogOrigin::Framework,
                       logger::LogBoundary::Instance,
                       "core.socket.stream",
                       instanceName.empty() ? std::nullopt : std::optional<std::string>(std::move(instanceName)),
                       logger::LogRole::Server,
                       std::nullopt) {
        }
        std::uint64_t allocateConnectionId() noexcept {
            return ++connectionsCreated;
        }
        void emitTerminationSummary(logger::BoundaryLogger::Sink sink) const {
            logScope.logger(std::move(sink)).info("Instance terminated: connections={} retries={}", connectionsCreated, flowController.getRetryCount());
        }
        core::socket::stream::ServerFlowController flowController;
        logger::LogScopeOwner logScope;
        std::uint64_t connectionsCreated{0};
    };

    struct ClientEndpointProbe {
        explicit ClientEndpointProbe(std::string instanceName)
            : flowController(instanceName)
            , logScope(logger::LogOrigin::Framework,
                       logger::LogBoundary::Instance,
                       "core.socket.stream",
                       instanceName.empty() ? std::nullopt : std::optional<std::string>(std::move(instanceName)),
                       logger::LogRole::Client,
                       std::nullopt) {
        }
        std::uint64_t allocateConnectionId() noexcept {
            return ++connectionsCreated;
        }
        void emitTerminationSummary(logger::BoundaryLogger::Sink sink) const {
            logScope.logger(std::move(sink)).info("Instance terminated: connections={} retries={} reconnects={}",
                                                  connectionsCreated,
                                                  flowController.getRetryCount(),
                                                  flowController.getReconnectCount());
        }
        ExposedClientFlowController flowController;
        logger::LogScopeOwner logScope;
        std::uint64_t connectionsCreated{0};
    };

    std::size_t countSummaries(const std::vector<logger::LogRecord>& records) {
        std::size_t count = 0;
        for (const logger::LogRecord& record : records) {
            if (record.message.rfind("Instance terminated:", 0) == 0) {
                ++count;
            }
        }
        return count;
    }
} // namespace

int main() {
    tests::support::TestResult result;

    ExposedRetryFlowController retryFlow;
    std::uint64_t retrySeenByObserver = 0;
    retryFlow.setOnFlowRetry([&](ExposedRetryFlowController* flow) {
        retrySeenByObserver = flow->getRetryCount();
    });
    result.expectEqual(0, static_cast<int>(retryFlow.getRetryCount()), "retryCount starts at zero");
    retryFlow.dispatchRetry();
    result.expectEqual(1, static_cast<int>(retryFlow.getRetryCount()), "retryCount increments once per dispatched retry");
    result.expectEqual(1, static_cast<int>(retrySeenByObserver), "retry observer sees incremented retryCount");

    ExposedClientFlowController reconnectFlow;
    std::uint64_t reconnectSeenByObserver = 0;
    reconnectFlow.setOnFlowReconnect([&](core::socket::stream::ClientFlowController* flow) {
        reconnectSeenByObserver = flow->getReconnectCount();
    });
    result.expectEqual(0, static_cast<int>(reconnectFlow.getReconnectCount()), "reconnectCount starts at zero");
    reconnectFlow.dispatchReconnect();
    result.expectEqual(1, static_cast<int>(reconnectFlow.getReconnectCount()), "reconnectCount increments once per dispatched reconnect");
    result.expectEqual(1, static_cast<int>(reconnectSeenByObserver), "reconnect observer sees incremented reconnectCount");

    ServerEndpointProbe server("endpoint-summary-server");
    const std::uint64_t firstServerId = server.allocateConnectionId();
    const std::uint64_t secondServerId = server.allocateConnectionId();
    result.expectEqual(1, static_cast<int>(firstServerId), "first server connection receives conn=1");
    result.expectEqual(2, static_cast<int>(secondServerId), "second server connection receives conn=2");
    result.expectTrue(firstServerId != secondServerId, "descriptor reuse does not imply connection ID reuse");

    std::vector<logger::LogRecord> serverRecords;
    if (server.flowController.terminateFlow()) {
        server.emitTerminationSummary([&](logger::LogRecord record) {
            serverRecords.push_back(std::move(record));
        });
    }
    if (server.flowController.terminateFlow()) {
        server.emitTerminationSummary([&](logger::LogRecord record) {
            serverRecords.push_back(std::move(record));
        });
    }
    result.expectEqual(1, static_cast<int>(countSummaries(serverRecords)), "server terminal retry rejection emits exactly one summary");
    result.expectTrue(serverRecords.size() == 1 && serverRecords[0].level == logger::LogLevel::Info &&
                          serverRecords[0].boundary == logger::LogBoundary::Instance && serverRecords[0].role == logger::LogRole::Server &&
                          serverRecords[0].instance == "endpoint-summary-server" && !serverRecords[0].connection &&
                          serverRecords[0].message == "Instance terminated: connections=2 retries=0",
                      "server summary is Info, instance-scoped, server role, with no connection field");

    std::vector<logger::LogRecord> acceptedRecords;
    ServerEndpointProbe acceptedServer("endpoint-accepted-server");
    acceptedServer.flowController.setOnFlowRetry([&](core::socket::stream::ServerFlowController*) {});
    result.expectTrue(acceptedRecords.empty(), "accepted retry emits no summary when terminateFlow is not called");

    ClientEndpointProbe connectClient("endpoint-connect-summary-client");
    connectClient.allocateConnectionId();
    std::vector<logger::LogRecord> connectClientRecords;
    if (connectClient.flowController.terminateFlow()) {
        connectClient.emitTerminationSummary([&](logger::LogRecord record) {
            connectClientRecords.push_back(std::move(record));
        });
    }
    if (connectClient.flowController.terminateFlow()) {
        connectClient.emitTerminationSummary([&](logger::LogRecord record) {
            connectClientRecords.push_back(std::move(record));
        });
    }
    result.expectEqual(1, static_cast<int>(countSummaries(connectClientRecords)), "client terminal connect-retry rejection emits exactly one summary");
    result.expectTrue(connectClientRecords.size() == 1 && connectClientRecords[0].level == logger::LogLevel::Info &&
                          connectClientRecords[0].boundary == logger::LogBoundary::Instance &&
                          connectClientRecords[0].role == logger::LogRole::Client &&
                          connectClientRecords[0].instance == "endpoint-connect-summary-client" && !connectClientRecords[0].connection &&
                          connectClientRecords[0].message == "Instance terminated: connections=1 retries=0 reconnects=0",
                      "client summary is Info, instance-scoped, client role, with no connection field");

    ClientEndpointProbe reconnectClient("endpoint-reconnect-summary-client");
    reconnectClient.allocateConnectionId();
    reconnectClient.flowController.dispatchReconnect();
    std::vector<logger::LogRecord> reconnectClientRecords;
    if (reconnectClient.flowController.terminateFlow()) {
        reconnectClient.emitTerminationSummary([&](logger::LogRecord record) {
            reconnectClientRecords.push_back(std::move(record));
        });
    }
    if (reconnectClient.flowController.terminateFlow()) {
        reconnectClient.emitTerminationSummary([&](logger::LogRecord record) {
            reconnectClientRecords.push_back(std::move(record));
        });
    }
    result.expectEqual(1,
                       static_cast<int>(countSummaries(reconnectClientRecords)),
                       "client reconnect rejection during RUNNING emits exactly one summary");
    result.expectTrue(reconnectClientRecords.size() == 1 && reconnectClientRecords[0].level == logger::LogLevel::Info &&
                          reconnectClientRecords[0].boundary == logger::LogBoundary::Instance &&
                          reconnectClientRecords[0].role == logger::LogRole::Client &&
                          reconnectClientRecords[0].instance == "endpoint-reconnect-summary-client" && !reconnectClientRecords[0].connection &&
                          reconnectClientRecords[0].message == "Instance terminated: connections=1 retries=0 reconnects=1",
                      "client reconnect summary contains reconnect count and no connection field");

    std::vector<logger::LogRecord> shutdownRecords;
    ClientEndpointProbe shutdownClient("endpoint-shutdown-client");
    shutdownClient.allocateConnectionId();
    result.expectTrue(shutdownRecords.empty(), "framework shutdown emits no summary without the rejected-continuation terminateFlow path");

    std::vector<logger::LogRecord> acceptedReconnectRecords;
    ClientEndpointProbe acceptedClient("endpoint-accepted-client");
    acceptedClient.allocateConnectionId();
    acceptedClient.flowController.dispatchReconnect();
    acceptedClient.allocateConnectionId();
    result.expectEqual(2, static_cast<int>(acceptedClient.connectionsCreated), "accepted reconnect preserves one shared client sequence");
    result.expectTrue(acceptedReconnectRecords.empty(), "accepted reconnect emits no summary when continuation is accepted");

    return result.processResult();
}
