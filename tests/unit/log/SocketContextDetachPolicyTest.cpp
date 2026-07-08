#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {

    std::filesystem::path projectRoot() {
        if (const char* sourceDir = std::getenv("SNODEC_SOURCE_DIR")) {
            return sourceDir;
        }

        std::filesystem::path current = std::filesystem::current_path();
        while (!current.empty()) {
            if (std::filesystem::exists(current / "src" / "SemanticLog.h")) {
                return current;
            }
            current = current.parent_path();
        }

        return std::filesystem::current_path();
    }

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream input(path);
        std::ostringstream buffer;
        buffer << input.rdbuf();
        return buffer.str();
    }

    bool contains(std::string_view haystack, std::string_view needle) {
        return haystack.find(needle) != std::string_view::npos;
    }

    bool requireContains(std::string_view contents, std::string_view needle, const std::filesystem::path& path) {
        if (contains(contents, needle)) {
            return true;
        }

        std::cerr << "Missing required fragment in " << path << ": " << needle << '\n';
        return false;
    }

    bool requireNotContains(std::string_view contents, std::string_view needle, const std::filesystem::path& path) {
        if (!contains(contents, needle)) {
            return true;
        }

        std::cerr << "Forbidden fragment in " << path << ": " << needle << '\n';
        return false;
    }

} // namespace

int main() {
    const std::filesystem::path root = projectRoot();
    const auto socketConnectionPath = root / "src" / "core" / "socket" / "stream" / "SocketConnection.hpp";
    const auto socketContextHeaderPath = root / "src" / "core" / "socket" / "stream" / "SocketContext.h";
    const auto socketContextSourcePath = root / "src" / "core" / "socket" / "stream" / "SocketContext.cpp";
    const auto socketServerPath = root / "src" / "core" / "socket" / "stream" / "SocketServer.h";
    const auto socketClientPath = root / "src" / "core" / "socket" / "stream" / "SocketClient.h";

    const std::string socketConnection = readFile(socketConnectionPath);
    const std::string socketContextHeader = readFile(socketContextHeaderPath);
    const std::string socketContextSource = readFile(socketContextSourcePath);
    const std::string socketServer = readFile(socketServerPath);
    const std::string socketClient = readFile(socketClientPath);

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
