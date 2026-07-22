#include "SourcePolicyTestRoot.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

    bool requireContains(std::string_view contents, std::string_view needle, const std::filesystem::path& path) {
        if (source_policy::contains(contents, needle)) {
            return true;
        }

        std::cerr << "Missing required fragment in " << path << ": " << needle << '\n';
        return false;
    }

    bool requireNotContains(std::string_view contents, std::string_view needle, const std::filesystem::path& path) {
        if (!source_policy::contains(contents, needle)) {
            return true;
        }

        std::cerr << "Forbidden fragment in " << path << ": " << needle << '\n';
        return false;
    }

    std::string_view requireBlock(std::string_view contents,
                                  std::string_view beginMarker,
                                  std::string_view endMarker,
                                  const std::filesystem::path& path) {
        const std::size_t begin = contents.find(beginMarker);
        if (begin == std::string_view::npos) {
            std::cerr << "Missing block begin marker in " << path << ": " << beginMarker << '\n';
            return {};
        }
        const std::size_t end = contents.find(endMarker, begin);
        if (end == std::string_view::npos) {
            std::cerr << "Missing block end marker in " << path << ": " << endMarker << '\n';
            return {};
        }
        return contents.substr(begin, end - begin);
    }

    bool requireOrdered(std::string_view contents,
                        const std::vector<std::string_view>& needles,
                        const std::filesystem::path& path) {
        std::size_t offset = 0;
        for (const std::string_view needle : needles) {
            const std::size_t found = contents.find(needle, offset);
            if (found == std::string_view::npos) {
                std::cerr << "Missing ordered fragment in " << path << ": " << needle << '\n';
                return false;
            }
            offset = found + needle.size();
        }
        return true;
    }

} // namespace

int main() {
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        return 1;
    }
    const auto socketConnectionPath = root / "src" / "core" / "socket" / "stream" / "SocketConnection.hpp";
    const auto socketContextHeaderPath = root / "src" / "core" / "socket" / "stream" / "SocketContext.h";
    const auto socketContextSourcePath = root / "src" / "core" / "socket" / "stream" / "SocketContext.cpp";
    const auto socketServerPath = root / "src" / "core" / "socket" / "stream" / "SocketServer.h";
    const auto socketClientPath = root / "src" / "core" / "socket" / "stream" / "SocketClient.h";
    const auto echoContextPath = root / "src" / "apps" / "echo" / "model" / "EchoSocketContext.cpp";
    const auto tlsLegacyContextPath = root / "src" / "apps" / "tlslegacy" / "TlsLegacySocketContext.cpp";

    const std::string socketConnection = source_policy::readSourcePolicyFile(socketConnectionPath);
    const std::string socketContextHeader = source_policy::readSourcePolicyFile(socketContextHeaderPath);
    const std::string socketContextSource = source_policy::readSourcePolicyFile(socketContextSourcePath);
    const std::string socketServer = source_policy::readSourcePolicyFile(socketServerPath);
    const std::string socketClient = source_policy::readSourcePolicyFile(socketClientPath);
    const std::string echoContext = source_policy::readSourcePolicyFile(echoContextPath);
    const std::string tlsLegacyContext = source_policy::readSourcePolicyFile(tlsLegacyContextPath);

    bool ok = true;
    ok &= requireContains(socketConnection, "socketContext->detach(SocketContext::DetachReason::ContextSwitch);", socketConnectionPath);
    ok &= requireContains(socketConnection, "activeSocketContext->detach(SocketContext::DetachReason::ConnectionClose);", socketConnectionPath);
    const std::string_view switchBlock = requireBlock(socketConnection,
                                                      "// Perform a pending SocketContextSwitch",
                                                      "this->log().debug(\"SocketConnection: switch completed\");",
                                                      socketConnectionPath);
    ok &= !switchBlock.empty();
    ok &= requireOrdered(switchBlock,
                         {"socketContext->detach(SocketContext::DetachReason::ContextSwitch);",
                          "socketContext = newSocketContext;",
                          "newSocketContext = nullptr;",
                          "socketContext->attach();"},
                         socketConnectionPath);
    ok &= requireNotContains(switchBlock, "onDisconnect", socketConnectionPath);

    ok &= requireContains(socketContextHeader, "enum class DetachReason", socketContextHeaderPath);
    ok &= requireContains(socketContextHeader, "ContextSwitch", socketContextHeaderPath);
    ok &= requireContains(socketContextHeader, "ConnectionClose", socketContextHeaderPath);
    ok &= requireContains(socketContextHeader, "DetachReason getDetachReason() const noexcept", socketContextHeaderPath);
    ok &= requireContains(socketContextHeader, "detach(DetachReason reason)", socketContextHeaderPath);

    ok &= requireContains(socketContextSource, "currentDetachReason = reason;", socketContextSourcePath);
    ok &= requireContains(socketContextSource, "if (reason == DetachReason::ContextSwitch)", socketContextSourcePath);
    ok &= requireContains(socketContextSource, "Context detached for context switch", socketContextSourcePath);
    ok &= requireContains(socketContextSource, "Context Online Since", socketContextSourcePath);
    ok &= requireContains(socketContextSource, "Context Online Duration", socketContextSourcePath);
    ok &= requireContains(socketContextSource, "Context Total Queued", socketContextSourcePath);
    ok &= requireContains(socketContextSource, "Context Total Processed", socketContextSourcePath);
    ok &= requireContains(socketContextSource, "Context detached for connection close", socketContextSourcePath);
    ok &= requireNotContains(socketContextSource, "Total Sent: {}", socketContextSourcePath);
    const std::string misleadingQueuedAsSent = std::string{"Total "} + "Sent: {}\", " + "getTotalQueued()";
    ok &= requireNotContains(socketContextSource, misleadingQueuedAsSent, socketContextSourcePath);

    for (const auto& pathAndContents :
         std::vector<std::pair<std::filesystem::path, std::string>>{{echoContextPath, echoContext}, {tlsLegacyContextPath, tlsLegacyContext}}) {
        const auto& path = pathAndContents.first;
        const auto& contents = pathAndContents.second;
        ok &= requireContains(contents, "log().debug", path);
        ok &= requireContains(contents, "getDetachReason() == DetachReason::ContextSwitch", path);
        ok &= requireContains(contents, "context switch", path);
        ok &= requireContains(contents, "connection close", path);
        ok &= requireNotContains(contents, "appLog().debug() << \"Echo: context", path);
        ok &= requireNotContains(contents, "appLog().debug() << \"TLS legacy: context", path);
        ok &= requireNotContains(contents, "getConnectionName() << \": context", path);
    }

    for (const auto& pathAndContents :
         std::vector<std::pair<std::filesystem::path, std::string>>{{socketServerPath, socketServer}, {socketClientPath, socketClient}}) {
        const auto& path = pathAndContents.first;
        const auto& contents = pathAndContents.second;
        for (const std::string_view label :
             {"Online Duration", "Total Queued", "Total Sent", "Total Read", "Total Processed", "Write Delta", "Read Delta"}) {
            ok &= requireContains(contents, label, path);
        }
    }

    return ok ? 0 : 1;
}
