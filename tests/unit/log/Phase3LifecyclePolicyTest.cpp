#include "SourcePolicyTestRoot.h"

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

    std::string functionBody(std::string_view source, std::string_view begin, std::string_view end) {
        const std::size_t first = source.find(begin);
        const std::size_t last = first == std::string_view::npos ? std::string_view::npos : source.find(end, first + begin.size());
        return first == std::string_view::npos || last == std::string_view::npos ? std::string()
                                                                                 : std::string(source.substr(first, last - first));
    }
} // namespace

int main() {
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        return 1;
    }

    const auto serverHttpPath = root / "src/web/http/server/SocketContext.cpp";
    const auto clientHttpPath = root / "src/web/http/client/SocketContext.cpp";
    const auto eventSourcePath = root / "src/web/http/client/tools/EventSource.h";
    const auto webSocketPath = root / "src/web/websocket/SocketContextUpgrade.hpp";
    const auto subProtocolPath = root / "src/web/websocket/SubProtocol.hpp";
    const auto mqttPath = root / "src/iot/mqtt/Mqtt.cpp";
    const auto mqttWsPath = root / "src/iot/mqtt/SubProtocol.hpp";
    const auto codexPath = root / "src/ai/openai/codex/AppServerClient.cpp";
    const auto reducerPath = root / "src/ai/openai/codex/backend/Reducer.cpp";
    const auto databasePath = root / "src/database/mariadb/MariaDBConnection.cpp";

    const std::string serverHttp = source_policy::readSourcePolicyFile(serverHttpPath);
    const std::string clientHttp = source_policy::readSourcePolicyFile(clientHttpPath);
    const std::string eventSource = source_policy::readSourcePolicyFile(eventSourcePath);
    const std::string webSocket = source_policy::readSourcePolicyFile(webSocketPath);
    const std::string subProtocol = source_policy::readSourcePolicyFile(subProtocolPath);
    const std::string mqtt = source_policy::readSourcePolicyFile(mqttPath);
    const std::string mqttWs = source_policy::readSourcePolicyFile(mqttWsPath);
    const std::string codex = source_policy::readSourcePolicyFile(codexPath);
    const std::string reducer = source_policy::readSourcePolicyFile(reducerPath);
    const std::string database = source_policy::readSourcePolicyFile(databasePath);

    bool ok = true;
    ok &= require(serverHttp + clientHttp, "request started", root / "src/web/http");
    for (const auto outcome : {"\"completed\"", "\"failed\"", "\"aborted\""}) {
        ok &= require(serverHttp + clientHttp, outcome, root / "src/web/http");
    }
    ok &= forbid(serverHttp + clientHttp, "Request start", root / "src/web/http");
    ok &= forbid(serverHttp + clientHttp, "Request (\" << request->count << \") completed", root / "src/web/http");

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
    ok &= forbid(mqttWs, "WsMqtt: connected", mqttWsPath);
    ok &= forbid(mqttWs, "WsMqtt: disconnected", mqttWsPath);

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
