#include "tests/support/TestResult.h"
#include "tests/unit/log/SourcePolicyTestRoot.h"

#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

namespace {
    struct Pattern {
        std::string name;
        std::regex expression;
    };

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream input(path);
        std::ostringstream buffer;
        buffer << input.rdbuf();
        return buffer.str();
    }

    bool isProductionSource(const std::filesystem::path& path) {
        const std::string ext = path.extension().string();
        return ext == ".cpp" || ext == ".h" || ext == ".hpp";
    }

    bool isScopedSemanticLoggerSurface(const std::filesystem::path& relative) {
        const std::string path = relative.generic_string();
        return path == "src/core/socket/stream/SocketContext.cpp" || path == "src/core/socket/stream/SocketConnection.cpp" ||
               path == "src/core/socket/stream/SocketConnection.hpp" || path == "src/core/socket/stream/SocketClient.h" ||
               path == "src/core/socket/stream/SocketServer.h" || path == "src/core/socket/stream/tls/SocketAcceptor.hpp" ||
               path == "src/core/socket/stream/tls/SocketConnector.hpp" || path == "src/core/socket/stream/tls/SocketConnection.hpp" ||
               path == "src/web/http/client/SocketContext.cpp" || path == "src/web/http/server/SocketContext.cpp";
    }

    std::string lineForOffset(const std::string& text, std::size_t offset) {
        std::size_t line = 1;
        for (std::size_t i = 0; i < offset && i < text.size(); ++i) {
            if (text[i] == '\n') {
                ++line;
            }
        }
        return std::to_string(line);
    }
} // namespace

int main() {
    tests::support::TestResult result;
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        result.expectTrue(false, "sourcePolicyProjectRoot() must not return an empty path");
        return result.processResult();
    }

    const std::vector<Pattern> patterns = {
        {"frameworkLog stream prepends socket connection identity",
         std::regex(
             R"(frameworkLog\s*\(\s*\)\s*\.\s*(trace|debug|info|warn|error|critical)\s*\(\s*\)\s*<<\s*(this->)?getSocketConnection\s*\(\s*\)\s*->\s*getConnectionName\s*\()")},
        {"captured frameworkLog stream prepends socket connection identity",
         std::regex(
             R"(([A-Za-z_][A-Za-z0-9_]*->)?frameworkLog\s*\(\s*\)\s*\.\s*(trace|debug|info|warn|error|critical)\s*\(\s*\)\s*<<\s*([A-Za-z_][A-Za-z0-9_]*->)?getSocketConnection\s*\(\s*\)\s*->\s*getConnectionName\s*\()")},
        {"object log stream prepends current connection identity",
         std::regex(
             R"((^|[^A-Za-z0-9_:])log\s*\(\s*\)\s*\.\s*(trace|debug|info|warn|error|critical)\s*\(\s*\)\s*<<\s*(this->)?getConnectionName\s*\()")},
        {"object log stream prepends current instance identity",
         std::regex(
             R"((^|[^A-Za-z0-9_:])log\s*\(\s*\)\s*\.\s*(trace|debug|info|warn|error|critical)\s*\(\s*\)\s*<<\s*(this->)?getInstanceName\s*\()")},
        {"object log format prepends current connection identity",
         std::regex(
             R"((this->)?log\s*\(\s*\)\s*\.\s*(trace|debug|info|warn|error|critical)\s*\(\s*"\{\}[^"\n]*"\s*,\s*(this->)?getConnectionName\s*\()")},
        {"object log format prepends current instance identity",
         std::regex(
             R"((this->)?log\s*\(\s*\)\s*\.\s*(trace|debug|info|warn|error|critical)\s*\(\s*"\{\}[^"\n]*"\s*,\s*(config->|this->)?getInstanceName\s*\()")},
        {"SocketConnection scoped log format prepends Super connection identity",
         std::regex(
             R"(SocketConnection::log\s*\(\s*\)\s*\.\s*(trace|debug|info|warn|error|critical)\s*\(\s*"\{\}[^"\n]*"\s*,\s*Super::getConnectionName\s*\()")},
        {"local frameworkLog variable prepends socket connection identity",
         std::regex(
             R"(auto\s+([A-Za-z_][A-Za-z0-9_]*)\s*=\s*frameworkLog\s*\(\s*\)\s*;[\s\S]{0,400}\b\1\s*\.\s*(trace|debug|info|warn|error|critical)\s*\(\s*\)\s*<<\s*(this->)?getSocketConnection\s*\(\s*\)\s*->\s*getConnectionName\s*\()")},
    };

    for (const auto& entry : std::filesystem::recursive_directory_iterator(root / "src")) {
        if (!entry.is_regular_file() || !isProductionSource(entry.path())) {
            continue;
        }

        const std::filesystem::path relative = std::filesystem::relative(entry.path(), root);
        if (!isScopedSemanticLoggerSurface(relative)) {
            continue;
        }

        const std::string text = readFile(entry.path());
        for (const Pattern& pattern : patterns) {
            std::smatch match;
            if (std::regex_search(text, match, pattern.expression)) {
                result.expectTrue(false,
                                  relative.generic_string() + ":" + lineForOffset(text, static_cast<std::size_t>(match.position())) + ": " +
                                      pattern.name);
            }
        }
    }

    return result.processResult();
}
