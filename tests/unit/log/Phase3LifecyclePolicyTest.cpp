#include "SourcePolicyTestRoot.h"

#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

namespace {
    bool require(std::string_view source, std::string_view fragment, const std::filesystem::path& path) {
        if (source.find(fragment) != std::string_view::npos) {
            return true;
        }
        std::cerr << "Missing Phase 3 lifecycle policy in " << path << ": " << fragment << '\n';
        return false;
    }

    bool forbid(std::string_view source, std::string_view fragment, const std::filesystem::path& path) {
        if (source.find(fragment) == std::string_view::npos) {
            return true;
        }
        std::cerr << "Forbidden Phase 3 lifecycle policy in " << path << ": " << fragment << '\n';
        return false;
    }

    bool requireCount(std::string_view source, std::string_view fragment, std::size_t expected, const std::filesystem::path& path) {
        std::size_t count = 0;
        for (std::size_t position = 0; (position = source.find(fragment, position)) != std::string_view::npos;
             position += fragment.size()) {
            ++count;
        }
        if (count == expected) {
            return true;
        }
        std::cerr << "Unexpected Phase 3 lifecycle policy count in " << path << ": " << fragment << " (expected " << expected << ", got "
                  << count << ")\n";
        return false;
    }

    std::string functionBody(std::string_view source, std::string_view begin, std::string_view end) {
        const std::size_t first = source.find(begin);
        const std::size_t last = first == std::string_view::npos ? std::string_view::npos : source.find(end, first + begin.size());
        return first == std::string_view::npos || last == std::string_view::npos ? std::string()
                                                                                 : std::string(source.substr(first, last - first));
    }

    std::string enclosingBlockContaining(std::string_view source, std::string_view marker) {
        const std::size_t markerPosition = source.find(marker);
        const std::size_t blockBegin =
            markerPosition == std::string_view::npos ? std::string_view::npos : source.rfind('{', markerPosition);
        if (blockBegin == std::string_view::npos) {
            return {};
        }

        std::size_t depth = 0;
        for (std::size_t position = blockBegin; position < source.size(); ++position) {
            if (source[position] == '{') {
                ++depth;
            } else if (source[position] == '}' && --depth == 0) {
                return std::string(source.substr(blockBegin, position - blockBegin + 1));
            }
        }
        return {};
    }

    std::string withoutWhitespace(std::string_view source) {
        std::string compact;
        compact.reserve(source.size());
        for (const unsigned char character : source) {
            if (std::isspace(character) == 0) {
                compact.push_back(static_cast<char>(character));
            }
        }
        return compact;
    }

    bool privateDeclaration(std::string_view source, std::string_view declaration, const std::filesystem::path& path) {
        const std::size_t declarationPosition = source.find(declaration);
        if (declarationPosition == std::string_view::npos) {
            std::cerr << "Missing Phase 3 private declaration in " << path << ": " << declaration << '\n';
            return false;
        }

        const std::size_t publicPosition = source.rfind("public:", declarationPosition);
        const std::size_t protectedPosition = source.rfind("protected:", declarationPosition);
        const std::size_t privatePosition = source.rfind("private:", declarationPosition);
        const bool isPrivate = privatePosition != std::string_view::npos &&
                               (publicPosition == std::string_view::npos || privatePosition > publicPosition) &&
                               (protectedPosition == std::string_view::npos || privatePosition > protectedPosition);
        if (!isPrivate) {
            std::cerr << "Phase 3 logging helper is not private in " << path << ": " << declaration << '\n';
        }
        return isPrivate;
    }
} // namespace

int main() {
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        return 1;
    }

    const auto serverHttpPath = root / "src/web/http/server/SocketContext.cpp";
    const auto clientHttpPath = root / "src/web/http/client/SocketContext.cpp";
    const auto serverResponsePath = root / "src/web/http/server/Response.cpp";
    const auto serverResponseHeaderPath = root / "src/web/http/server/Response.h";
    const auto clientRequestPath = root / "src/web/http/client/Request.cpp";
    const auto clientRequestHeaderPath = root / "src/web/http/client/Request.h";
    const auto eventSourcePath = root / "src/web/http/client/tools/EventSource.h";
    const auto webSocketPath = root / "src/web/websocket/SocketContextUpgrade.hpp";
    const auto subProtocolPath = root / "src/web/websocket/SubProtocol.hpp";
    const auto mqttHeaderPath = root / "src/iot/mqtt/Mqtt.h";
    const auto mqttPath = root / "src/iot/mqtt/Mqtt.cpp";
    const auto mqttWsPath = root / "src/iot/mqtt/SubProtocol.hpp";
    const auto mqttClientPath = root / "src/iot/mqtt/client/Mqtt.cpp";
    const auto mqttClientHeaderPath = root / "src/iot/mqtt/client/Mqtt.h";
    const auto mqttServerPath = root / "src/iot/mqtt/server/Mqtt.cpp";
    const auto mqttServerHeaderPath = root / "src/iot/mqtt/server/Mqtt.h";
    const auto mqttBrokerPath = root / "src/iot/mqtt/server/broker/Broker.cpp";
    const auto mqttRetainPath = root / "src/iot/mqtt/server/broker/RetainTree.cpp";
    const auto mqttSessionPath = root / "src/iot/mqtt/server/broker/Session.cpp";
    const auto mqttSubscriptionPath = root / "src/iot/mqtt/server/broker/SubscriptionTree.cpp";
    const auto codexPath = root / "src/ai/openai/codex/AppServerClient.cpp";
    const auto backendEventPath = root / "src/ai/openai/codex/backend/BackendEvent.h";
    const auto backendStatePath = root / "src/ai/openai/codex/backend/BackendState.h";
    const auto reducerPath = root / "src/ai/openai/codex/backend/Reducer.cpp";
    const auto databasePath = root / "src/database/mariadb/MariaDBConnection.cpp";

    const std::string serverHttp = source_policy::readSourcePolicyFile(serverHttpPath);
    const std::string clientHttp = source_policy::readSourcePolicyFile(clientHttpPath);
    const std::string serverResponse = source_policy::readSourcePolicyFile(serverResponsePath);
    const std::string serverResponseHeader = source_policy::readSourcePolicyFile(serverResponseHeaderPath);
    const std::string clientRequest = source_policy::readSourcePolicyFile(clientRequestPath);
    const std::string clientRequestHeader = source_policy::readSourcePolicyFile(clientRequestHeaderPath);
    const std::string eventSource = source_policy::readSourcePolicyFile(eventSourcePath);
    const std::string webSocket = source_policy::readSourcePolicyFile(webSocketPath);
    const std::string subProtocol = source_policy::readSourcePolicyFile(subProtocolPath);
    const std::string mqttHeader = source_policy::readSourcePolicyFile(mqttHeaderPath);
    const std::string mqtt = source_policy::readSourcePolicyFile(mqttPath);
    const std::string mqttWs = source_policy::readSourcePolicyFile(mqttWsPath);
    const std::string mqttClient = source_policy::readSourcePolicyFile(mqttClientPath);
    const std::string mqttClientHeader = source_policy::readSourcePolicyFile(mqttClientHeaderPath);
    const std::string mqttServer = source_policy::readSourcePolicyFile(mqttServerPath);
    const std::string mqttServerHeader = source_policy::readSourcePolicyFile(mqttServerHeaderPath);
    const std::string mqttBroker = source_policy::readSourcePolicyFile(mqttBrokerPath);
    const std::string mqttRetain = source_policy::readSourcePolicyFile(mqttRetainPath);
    const std::string mqttSession = source_policy::readSourcePolicyFile(mqttSessionPath);
    const std::string mqttSubscription = source_policy::readSourcePolicyFile(mqttSubscriptionPath);
    const std::string codex = source_policy::readSourcePolicyFile(codexPath);
    const std::string backendEvent = source_policy::readSourcePolicyFile(backendEventPath);
    const std::string backendState = source_policy::readSourcePolicyFile(backendStatePath);
    const std::string reducer = source_policy::readSourcePolicyFile(reducerPath);
    const std::string database = source_policy::readSourcePolicyFile(databasePath);

    bool ok = true;
    ok &= require(serverHttp + clientHttp, "request started", root / "src/web/http");
    for (const auto outcome : {"\"completed\"", "\"failed\"", "\"aborted\""}) {
        ok &= require(serverHttp + clientHttp, outcome, root / "src/web/http");
    }
    ok &= forbid(serverHttp + clientHttp, "Request start", root / "src/web/http");
    ok &= forbid(serverHttp + clientHttp, "Request (\" << request->count << \") completed", root / "src/web/http");

    const std::string serverUpgrade = functionBody(serverResponse, "void Response::upgrade", "void Response::sendFile");
    const std::string clientUpgrade = functionBody(clientRequest, "bool MasterRequest::upgrade(", "bool MasterRequest::requestEventSource");
    const std::string clientEventSourceRequest =
        functionBody(clientRequest, "bool MasterRequest::requestEventSource", "bool MasterRequest::sendFile");
    const std::string clientExecuteUpgrade =
        functionBody(clientRequest, "bool MasterRequest::executeUpgrade", "bool MasterRequest::executeEnd");
    const std::string missingUpgradeRequest = enclosingBlockContaining(serverUpgrade, "Upgrade request has gone away");
    const std::string disconnectedUpgrade = enclosingBlockContaining(serverUpgrade, "Unexpected disconnect during upgrade");
    ok &= requireCount(serverUpgrade, "semantic::httpServerLog(*", 1, serverResponsePath);
    ok &= requireCount(serverUpgrade, "semantic::httpServerLog()", 1, serverResponsePath);
    ok &= forbid(serverUpgrade, "connectionName", serverResponsePath);
    ok &= forbid(serverUpgrade, "getConnectionName()", serverResponsePath);
    ok &= forbid(serverUpgrade, "HTTP:", serverResponsePath);
    ok &= forbid(serverUpgrade, "HTTP upgrade:", serverResponsePath);
    for (const auto payload : {"Initiating upgrade:",
                               "SocketContextUpgradeFactory create success",
                               "Response to upgrade request:",
                               "Upgrade bootstrap",
                               "Protocol selected:"}) {
        ok &= require(serverUpgrade, payload, serverResponsePath);
    }
    ok &= require(withoutWhitespace(missingUpgradeRequest), "set(\"Connection\",\"close\").status(500);", serverResponsePath);
    ok &= require(withoutWhitespace(missingUpgradeRequest), "status({});return;", serverResponsePath);
    ok &= forbid(missingUpgradeRequest, "request->", serverResponsePath);
    ok &= forbid(missingUpgradeRequest, "SocketContextUpgradeFactorySelector", serverResponsePath);
    ok &= forbid(missingUpgradeRequest, "setSocketContext", serverResponsePath);

    ok &= requireCount(disconnectedUpgrade, "semantic::httpServerLog()", 1, serverResponsePath);
    ok &= forbid(disconnectedUpgrade, "semantic::httpServerLog(*", serverResponsePath);
    ok &= forbid(disconnectedUpgrade, "socketContext", serverResponsePath);
    ok &= forbid(disconnectedUpgrade, "connectionName", serverResponsePath);
    ok &= forbid(disconnectedUpgrade, "getConnectionName()", serverResponsePath);
    for (const auto prefix : {"Upgrade:", "HTTP:", "HTTP upgrade:"}) {
        ok &= forbid(disconnectedUpgrade, prefix, serverResponsePath);
    }
    ok &= require(disconnectedUpgrade, "Unexpected disconnect during upgrade", serverResponsePath);

    ok &= requireCount(clientUpgrade + clientEventSourceRequest + clientExecuteUpgrade, "semantic::httpClientLog(*", 3, clientRequestPath);
    ok &= forbid(clientUpgrade + clientEventSourceRequest + clientExecuteUpgrade, "semantic::httpClientLog()", clientRequestPath);
    ok &= forbid(clientUpgrade + clientEventSourceRequest + clientExecuteUpgrade, "connectionName", clientRequestPath);
    ok &= forbid(clientUpgrade + clientEventSourceRequest + clientExecuteUpgrade, "getConnectionName()", clientRequestPath);
    ok &= forbid(clientUpgrade + clientEventSourceRequest + clientExecuteUpgrade, "HTTP:", clientRequestPath);
    ok &= forbid(clientUpgrade + clientEventSourceRequest + clientExecuteUpgrade, "HTTP upgrade:", clientRequestPath);

    ok &= requireCount(eventSource, "httpClientLog(*", 8, eventSourcePath);
    ok &= forbid(eventSource, "socketConnection->getConnectionName()", eventSourcePath);
    ok &= requireCount(eventSource, "getConnectionName()", 1, eventSourcePath);
    ok &= requireCount(eventSource, "\": OnRequestEnd\"", 1, eventSourcePath);
    ok &= forbid(serverResponseHeader + clientRequestHeader + eventSource, "BoundaryLogger", root / "src/web/http");
    ok &= forbid(serverResponseHeader + clientRequestHeader + eventSource, "LogScopeOwner", root / "src/web/http");

    for (const auto event : {"event source started",
                             "event stream established",
                             "event source reconnect scheduled",
                             "event source reconnect dispatched",
                             "event source reconnect cancelled",
                             "event source stopped"}) {
        ok &= require(eventSource, event, eventSourcePath);
    }

    ok &= require(webSocket, "websocket established", webSocketPath);
    ok &= require(webSocket, "websocket ended", webSocketPath);
    ok &= require(subProtocol, "subprotocol started", subProtocolPath);
    ok &= require(subProtocol, "subprotocol stopped", subProtocolPath);

    for (const auto event : {"mqtt protocol started",
                             "mqtt protocol stopped",
                             "mqtt session established",
                             "mqtt session resumed",
                             "mqtt session rejected",
                             "mqtt session ended"}) {
        ok &= require(mqtt, event, mqttPath);
    }
    ok &= forbid(mqtt, "MQTT: Connected", mqttPath);
    ok &= forbid(mqtt, "MQTT: Disconnected", mqttPath);
    ok &= privateDeclaration(mqttHeader, "sessionEstablished(bool resumed)", mqttHeaderPath);
    ok &= privateDeclaration(mqttHeader, "sessionRejected(const std::string& rejectedClientId", mqttHeaderPath);
    ok &= forbid(mqttWs, "WsMqtt: connected", mqttWsPath);
    ok &= forbid(mqttWs, "WsMqtt: disconnected", mqttWsPath);

    const std::string mqttProduction = mqtt + mqttWs + mqttClient + mqttServer + mqttBroker + mqttRetain + mqttSession + mqttSubscription;
    for (const auto prefix : {"MQTT:", "MQTT Client:", "MQTT Broker:", "WsMqtt:"}) {
        ok &= forbid(mqttProduction, prefix, root / "src/iot/mqtt");
    }
    ok &= forbid(withoutWhitespace(mqtt), "<<connectionName", mqttPath);
    ok &= forbid(withoutWhitespace(mqtt), "emitPresentedTrace(log(),connectionName", mqttPath);

    const std::string clientBound = functionBody(mqttClient, "bool Mqtt::onSignal", "} // namespace iot::mqtt::client");
    ok &= forbid(withoutWhitespace(clientBound), "<<connectionName", mqttClientPath);
    ok &= require(mqttClient, "\"iot.mqtt.client\"", mqttClientPath);
    ok &= require(mqttClient, "logger::LogRole::Client", mqttClientPath);
    ok &= forbid(mqttClientHeader, "clientLog(", mqttClientHeaderPath);
    ok &= require(mqttClient, "{}: ... Could not read session store", mqttClientPath);
    ok &= require(mqttClient, "{}: Could not write session store", mqttClientPath);

    const std::string serverNegotiation = functionBody(mqttServer, "bool Mqtt::initSession", "void Mqtt::releaseSession");
    const std::string serverLive = functionBody(mqttServer, "void Mqtt::distributePublish", "} // namespace iot::mqtt::server");
    ok &= forbid(withoutWhitespace(serverNegotiation + serverLive), "<<connectionName", mqttServerPath);
    ok &= require(mqttServer, "\"iot.mqtt.server\"", mqttServerPath);
    ok &= require(mqttServer, "logger::LogRole::Server", mqttServerPath);
    ok &= forbid(mqttServerHeader, "serverLog(", mqttServerHeaderPath);
    ok &= privateDeclaration(mqttServerHeader, "releaseSession(logger::BoundaryLogger", mqttServerHeaderPath);
    ok &= require(withoutWhitespace(mqttServer), "connectionName+\":\"", mqttServerPath);
    ok &= require(withoutWhitespace(mqttServer), "releaseSession(serverLog(*this),{})", mqttServerPath);

    for (const auto domainField : {"client=", "Topic:", "PacketIdentifier", "QoS:", "DUP:", "Retain:"}) {
        ok &= require(mqttProduction, domainField, root / "src/iot/mqtt");
    }
    ok &= forbid(mqttProduction, "transport connected", root / "src/iot/mqtt");
    ok &= forbid(mqttProduction, "transport disconnected", root / "src/iot/mqtt");

    for (const auto event : {"app-server session started",
                             "app-server session stopped",
                             "app-server session start failed",
                             "request started",
                             "request cancelled"}) {
        ok &= require(codex, event, codexPath);
    }
    ok &= require(codex, "request {}: id={}", codexPath);
    ok &= require(codex, "\"failed\" : \"completed\"", codexPath);
    const std::string notificationBody = functionBody(codex, "void dispatchNotification", "void dispatchServerRequest");
    ok &= forbid(notificationBody, "request started", codexPath);
    ok &= forbid(notificationBody, "request completed", codexPath);
    for (const auto field : {"lifecycleStart", "creationLogged", "lifecycleStarted", "lifecycleTerminalLogged"}) {
        ok &= forbid(backendEvent + backendState, field, root / "src/ai/openai/codex/backend");
    }
    ok &= require(reducer, "thread created", reducerPath);
    ok &= require(reducer, "turn started", reducerPath);
    ok &= require(reducer, "turn failed", reducerPath);
    ok &= require(reducer, "\"cancelled\" : \"completed\"", reducerPath);
    ok &= forbid(reducer, "thread closed", reducerPath);

    for (const auto event : {"database session established", "database session ended", "database request started", "database request "}) {
        ok &= require(database, event, databasePath);
    }
    ok &= require(database, "\"cancelled\" : \"failed\"", databasePath);
    ok &= require(database, "currentCommandFailed ? \"failed\" : \"completed\"", databasePath);

    return ok ? 0 : 1;
}
