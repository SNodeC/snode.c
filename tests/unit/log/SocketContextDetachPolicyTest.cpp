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

    const std::string socketConnection = source_policy::readSourcePolicyFile(socketConnectionPath);
    const std::string socketContextHeader = source_policy::readSourcePolicyFile(socketContextHeaderPath);
    const std::string socketContextSource = source_policy::readSourcePolicyFile(socketContextSourcePath);
    const std::string socketServer = source_policy::readSourcePolicyFile(socketServerPath);
    const std::string socketClient = source_policy::readSourcePolicyFile(socketClientPath);

    bool ok = true;
    ok &= requireContains(socketConnection, "socketContext->detach(SocketContext::DetachReason::ContextSwitch);", socketConnectionPath);
    ok &= requireContains(socketConnection, "socketContext->detach(SocketContext::DetachReason::ConnectionClose);", socketConnectionPath);

    ok &= requireContains(socketContextHeader, "enum class DetachReason", socketContextHeaderPath);
    ok &= requireContains(socketContextHeader, "ContextSwitch", socketContextHeaderPath);
    ok &= requireContains(socketContextHeader, "ConnectionClose", socketContextHeaderPath);
    ok &= requireContains(socketContextHeader, "detach(DetachReason reason)", socketContextHeaderPath);

    ok &= requireContains(socketContextSource, "if (reason == DetachReason::ContextSwitch)", socketContextSourcePath);
    ok &= requireContains(socketContextSource, "SocketContext: detached for context switch", socketContextSourcePath);
    ok &= requireContains(socketContextSource, "Context Online Since", socketContextSourcePath);
    ok &= requireContains(socketContextSource, "Context Online Duration", socketContextSourcePath);
    ok &= requireContains(socketContextSource, "Context Total Queued", socketContextSourcePath);
    ok &= requireContains(socketContextSource, "Context Total Processed", socketContextSourcePath);
    ok &= requireContains(socketContextSource, "SocketContext: detached for connection close", socketContextSourcePath);
    ok &= requireNotContains(socketContextSource, "Total Sent: {}", socketContextSourcePath);
    const std::string misleadingQueuedAsSent = std::string{"Total "} + "Sent: {}\", " + "getTotalQueued()";
    ok &= requireNotContains(socketContextSource, misleadingQueuedAsSent, socketContextSourcePath);

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
