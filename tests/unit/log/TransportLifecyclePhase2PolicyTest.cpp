#include "SourcePolicyTestRoot.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {
    bool require(std::string_view source, std::string_view fragment, const std::filesystem::path& path) {
        if (source.find(fragment) != std::string_view::npos) {
            return true;
        }
        std::cerr << "Missing Phase 2 fragment in " << path << ": " << fragment << '\n';
        return false;
    }

    bool forbid(std::string_view source, std::string_view fragment, const std::filesystem::path& path) {
        if (source.find(fragment) == std::string_view::npos) {
            return true;
        }
        std::cerr << "Forbidden Phase 2 fragment in " << path << ": " << fragment << '\n';
        return false;
    }

    bool ordered(std::string_view source, const std::vector<std::string_view>& fragments, const std::filesystem::path& path) {
        std::size_t offset = 0;
        for (const auto fragment : fragments) {
            const std::size_t position = source.find(fragment, offset);
            if (position == std::string_view::npos) {
                std::cerr << "Missing ordered Phase 2 fragment in " << path << ": " << fragment << '\n';
                return false;
            }
            offset = position + fragment.size();
        }
        return true;
    }

    std::string readSourceTree(const std::filesystem::path& root) {
        std::string source;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
            if (entry.is_regular_file()) {
                const auto extension = entry.path().extension();
                if (extension == ".cpp" || extension == ".h" || extension == ".hpp") {
                    source += source_policy::readSourcePolicyFile(entry.path());
                }
            }
        }
        return source;
    }
} // namespace

int main() {
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        return 1;
    }

    const auto connectionPath = root / "src/core/socket/stream/SocketConnection.hpp";
    const auto connectorPath = root / "src/core/socket/stream/SocketConnector.hpp";
    const auto acceptorPath = root / "src/core/socket/stream/SocketAcceptor.hpp";
    const auto clientPath = root / "src/core/socket/stream/SocketClient.h";
    const auto serverPath = root / "src/core/socket/stream/SocketServer.h";
    const auto tlsClientPath = root / "src/core/socket/stream/tls/SocketConnector.hpp";
    const auto tlsServerPath = root / "src/core/socket/stream/tls/SocketAcceptor.hpp";
    const auto codexPath = root / "src/ai/openai/codex/stdio/StdioTransport.cpp";
    const auto contextPath = root / "src/core/socket/stream/SocketContext.cpp";
    const auto mqttPath = root / "src/iot/mqtt/Mqtt.cpp";

    const std::string connection = source_policy::readSourcePolicyFile(connectionPath);
    const std::string connector = source_policy::readSourcePolicyFile(connectorPath);
    const std::string acceptor = source_policy::readSourcePolicyFile(acceptorPath);
    const std::string client = source_policy::readSourcePolicyFile(clientPath);
    const std::string server = source_policy::readSourcePolicyFile(serverPath);
    const std::string tlsClient = source_policy::readSourcePolicyFile(tlsClientPath);
    const std::string tlsServer = source_policy::readSourcePolicyFile(tlsServerPath);
    const std::string codex = source_policy::readSourcePolicyFile(codexPath);
    const std::string context = source_policy::readSourcePolicyFile(contextPath);
    const std::string coreSocketSources = readSourceTree(root / "src/core/socket");
    const std::string mqtt = source_policy::readSourcePolicyFile(mqttPath);

    bool ok = true;
    ok &= require(connector, "socketConnection->log().info(\"transport connected\")", connectorPath);
    ok &= require(acceptor, "socketConnection->log().info(\"transport connected\")", acceptorPath);
    ok &= require(connection, "Super::log().info(\"transport disconnected\")", connectionPath);
    ok &= forbid(connection, "log().debug(\"disconnected\")", connectionPath);
    ok &= forbid(coreSocketSources, ".debug(\"disconnected\")", root / "src/core/socket");
    ok &= forbid(coreSocketSources, ".info(\"disconnected\")", root / "src/core/socket");
    ok &= ordered(connection,
                  {"socketContext->detach(SocketContext::DetachReason::ConnectionClose)",
                   "onDisconnect();",
                   "Super::log().info(\"transport disconnected\")",
                   "delete this;"},
                  connectionPath);

    for (const auto outcome : {"connection attempt started",
                               "connection attempt succeeded",
                               "connection attempt failed",
                               "connection attempt timed out",
                               "connection attempt cancelled"}) {
        ok &= require(connector, outcome, connectorPath);
    }
    ok &= require(connector, "std::exchange(attemptActive, false)", connectorPath);
    ok &= require(connector, "log().debug(outcome)", connectorPath);

    ok &= require(acceptor, "log().info(\"listener started\")", acceptorPath);
    ok &= require(acceptor, "log().info(\"listener stopped\")", acceptorPath);
    ok &= require(acceptor, "log().debug(\"listener start failed\")", acceptorPath);
    ok &= ordered(acceptor, {"unobservedEvent()", "listener stopped", "destruct();"}, acceptorPath);

    for (const auto continuation : {"retry scheduled", "retry dispatched", "retry cancelled"}) {
        ok &= require(client, continuation, clientPath);
        ok &= require(server, continuation, serverPath);
    }
    for (const auto continuation : {"reconnect scheduled", "reconnect dispatched", "reconnect cancelled"}) {
        ok &= require(client, continuation, clientPath);
    }
    ok &= forbid(client, "log.info(\"Retry connect", clientPath);
    ok &= forbid(client, "log.info(\"Reconnect", clientPath);
    ok &= forbid(server, "log.info(\"Retry listen", serverPath);

    ok &= require(tlsClient, "log.info(\"transport ready\")", tlsClientPath);
    ok &= require(tlsServer, "log.info(\"transport ready\")", tlsServerPath);
    ok &= ordered(tlsClient, {"Handshake success", "transport ready", "onConnected(socketConnection)"}, tlsClientPath);
    ok &= ordered(tlsServer, {"Handshake success", "transport ready", "onConnected(socketConnection)"}, tlsServerPath);

    for (const auto processEvent : {"child process spawned",
                                    "child process exited",
                                    "child process terminated",
                                    "child process spawn failed",
                                    "stdio transport started",
                                    "stdio transport stopped",
                                    "stdio transport start failed"}) {
        ok &= require(codex, processEvent, codexPath);
    }
    ok &= require(codex, "childPid = -1;", codexPath);
    ok &= ordered(codex, {"child process spawned", "stdio transport started"}, codexPath);
    ok &= ordered(codex, {"child process exited", "childPid = -1", "stdio transport stopped"}, codexPath);

    ok &= require(context, "Context attached", contextPath);
    ok &= require(context, "Context detached for context switch", contextPath);
    ok &= require(context, "Context detached for connection close", contextPath);
    ok &= forbid(context, "transport connected", contextPath);
    ok &= forbid(context, "transport disconnected", contextPath);

    ok &= require(mqtt, "MQTT: Connected", mqttPath);
    ok &= require(mqtt, "MQTT: Disconnected", mqttPath);

    return ok ? 0 : 1;
}
