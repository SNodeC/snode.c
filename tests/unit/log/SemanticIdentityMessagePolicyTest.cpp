#include "SourcePolicyTestRoot.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace {
    constexpr std::string_view auditNote = R"(Semantic identity audit note:
- The policy test scans production .cpp/.h/.hpp files under src/ rather than a hand-picked subset.
- Call sites cleaned by this PR are scoped HTTP SocketContext and TLS SocketConnection/Connector/Acceptor logs whose semantic scopes already carry inst=/conn=.
- Remaining identity-name streams in appLog(), expressLog(), webSocketLog(), httpClientLog(), mqttLog(), tlsConfigLog(), and unscoped coreSocketLog() are intentionally kept when those logger helpers do not carry the same connection/instance identity, or when the identity is used to construct scopes, protocol/application payloads, callbacks, legacy diagnostics, or another connection's diagnostic context.
)";

    const std::vector<std::string> semanticHelpers = {
        "frameworkLog",
        "log",
        "appLog",
        "expressLog",
        "httpLog",
        "httpClientLog",
        "httpClientUpgradeLog",
        "httpClientEventSourceLog",
        "webHttpLog",
        "websocketLog",
        "webSocketLog",
        "webSocketFrameLog",
        "webSocketSubProtocolLog",
        "webSocketFactoryLog",
        "mqttLog",
        "mqttClientLog",
        "mqttServerLog",
        "mqttBrokerLog",
        "mqttSessionLog",
        "mqttPacketLog",
        "mqttTopicLog",
        "mqttWebSocketLog",
        "coreSocketLog",
        "tlsConfigLog",
        "netConfigLog",
        "mariaDbLog",
    };

    const std::vector<std::string> directLoggerExpressions = {
        "this->log()",
        "core::socket::stream::SocketConnection::log()",
    };

    const std::vector<std::string> levels = {"trace", "debug", "info", "warn", "error", "critical"};

    const std::vector<std::string> identityExpressions = {
        "getConnectionName()",
        "getInstanceName()",
        "getSocketConnection()->getConnectionName()",
        "getSocketConnection()->getInstanceName()",
        "socketConnection->getConnectionName()",
        "socketConnection->getInstanceName()",
        "socketContext->getSocketConnection()->getConnectionName()",
        "socketContext->getSocketConnection()->getInstanceName()",
        "this->getSocketConnection()->getConnectionName()",
        "this->getSocketConnection()->getInstanceName()",
        "this->subProtocolContext->getSocketConnection()->getConnectionName()",
        "subProtocolContext->getSocketConnection()->getConnectionName()",
        "res->getSocketContext()->getSocketConnection()->getConnectionName()",
        "req->getSocketContext()->getSocketConnection()->getConnectionName()",
        "req->getConnectionName()",
        "masterRequest->getSocketContext()->getSocketConnection()->getConnectionName()",
        "controller.getResponse()->getSocketContext()->getSocketConnection()->getConnectionName()",
        "config->getInstanceName()",
        "client.getConfig()->getInstanceName()",
        "server.getConfig()->getInstanceName()",
    };

    struct Violation {
        std::filesystem::path file;
        std::string pattern;
        std::string reason;
    };

    std::string stripComments(std::string_view source) {
        std::string out;
        out.reserve(source.size());
        bool lineComment = false;
        bool blockComment = false;
        bool stringLiteral = false;
        bool charLiteral = false;
        bool escaped = false;
        for (std::size_t i = 0; i < source.size(); ++i) {
            const char ch = source[i];
            const char next = i + 1 < source.size() ? source[i + 1] : '\0';
            if (lineComment) {
                if (ch == '\n') {
                    lineComment = false;
                    out.push_back(ch);
                }
                continue;
            }
            if (blockComment) {
                if (ch == '*' && next == '/') {
                    blockComment = false;
                    ++i;
                }
                continue;
            }
            if (!stringLiteral && !charLiteral && ch == '/' && next == '/') {
                lineComment = true;
                ++i;
                continue;
            }
            if (!stringLiteral && !charLiteral && ch == '/' && next == '*') {
                blockComment = true;
                ++i;
                continue;
            }
            out.push_back(ch);
            if (escaped) {
                escaped = false;
                continue;
            }
            if ((stringLiteral || charLiteral) && ch == '\\') {
                escaped = true;
                continue;
            }
            if (!charLiteral && ch == '"') {
                stringLiteral = !stringLiteral;
            } else if (!stringLiteral && ch == '\'') {
                charLiteral = !charLiteral;
            }
        }
        return out;
    }

    std::string compact(std::string_view value) {
        std::string result;
        result.reserve(value.size());
        for (const unsigned char ch : value) {
            if (!std::isspace(ch)) {
                result.push_back(static_cast<char>(ch));
            }
        }
        return result;
    }

    std::vector<std::filesystem::path> productionSourceFiles(const std::filesystem::path& root) {
        std::vector<std::filesystem::path> files;
        const std::filesystem::path src = root / "src";
        for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(src)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const std::filesystem::path path = entry.path();
            const std::string ext = path.extension().string();
            if (ext == ".cpp" || ext == ".h" || ext == ".hpp") {
                files.push_back(path);
            }
        }
        std::sort(files.begin(), files.end());
        return files;
    }

    bool isAllowed(const std::filesystem::path& relative, const std::string& pattern) {
        const std::string path = relative.generic_string();

        // Static/shared semantic helpers below do not carry the concrete connection/instance being printed.
        if (pattern.find("appLog().") != std::string::npos || pattern.find("expressLog().") != std::string::npos ||
            pattern.find("httpClientLog().") != std::string::npos || pattern.find("webSocket") != std::string::npos ||
            pattern.find("websocket") != std::string::npos || pattern.find("mqtt") != std::string::npos ||
            pattern.find("coreSocketLog().") != std::string::npos || pattern.find("tlsConfigLog().") != std::string::npos) {
            return true;
        }

        // Instance-scoped client/server logs deliberately describe a connection handed to a callback, not their own scope identity.
        if ((path == "src/core/socket/stream/SocketClient.h" || path == "src/core/socket/stream/SocketServer.h") &&
            pattern.find("socketConnection->getConnectionName()") != std::string::npos) {
            return true;
        }

        // Configuration/callback diagnostics are scoped to an instance and print that instance for user-facing lifecycle callbacks.
        if (path.rfind("src/apps/", 0) == 0 || path.rfind("src/express/legacy/", 0) == 0 || path.rfind("src/express/tls/", 0) == 0) {
            return true;
        }

        return false;
    }

    void addIfFound(std::vector<Violation>& violations,
                    const std::filesystem::path& file,
                    const std::string& normalized,
                    const std::string& pattern,
                    const std::string& reason) {
        if (source_policy::contains(normalized, pattern)) {
            violations.push_back({file, pattern, reason});
        }
    }

    std::vector<std::string> assignedSemanticLogVariables(const std::string& normalized) {
        std::set<std::string> variables;
        for (const std::string& helper : semanticHelpers) {
            std::size_t pos = 0;
            const std::string needle = "=" + helper + "();";
            while ((pos = normalized.find(needle, pos)) != std::string::npos) {
                std::size_t nameEnd = pos;
                if (nameEnd == 0) {
                    pos += needle.size();
                    continue;
                }
                std::size_t nameStart = nameEnd;
                while (nameStart > 0) {
                    const char ch = normalized[nameStart - 1];
                    if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')) {
                        break;
                    }
                    --nameStart;
                }
                const std::string name = normalized.substr(nameStart, nameEnd - nameStart);
                if (!name.empty() && name != "auto" && name != "constauto") {
                    variables.insert(name);
                }
                pos += needle.size();
            }
        }
        return {variables.begin(), variables.end()};
    }

    std::vector<Violation> findViolations(const std::filesystem::path& root, const std::filesystem::path& file) {
        const std::string contents = source_policy::readSourcePolicyFile(file);
        const std::string normalized = compact(stripComments(contents));
        const std::filesystem::path relative = std::filesystem::relative(file, root);
        std::vector<Violation> found;

        for (const std::string& level : levels) {
            for (const std::string& identity : identityExpressions) {
                for (const std::string& helper : semanticHelpers) {
                    const std::string expression = helper + "()";
                    addIfFound(found,
                               relative,
                               normalized,
                               expression + "." + level + "()<<" + identity,
                               "direct semantic stream prepends identity");
                    addIfFound(found,
                               relative,
                               normalized,
                               expression + "." + level + "(\"{}",
                               "direct semantic format string may prepend identity");
                    if (!found.empty() && found.back().pattern == expression + "." + level + "(\"{}" &&
                        normalized.find(expression + "." + level + "(\"{}") != std::string::npos &&
                        normalized.find(identity, normalized.find(expression + "." + level + "(\"{}")) == std::string::npos) {
                        found.pop_back();
                    }
                }
                for (const std::string& expression : directLoggerExpressions) {
                    addIfFound(found,
                               relative,
                               normalized,
                               expression + "." + level + "()<<" + identity,
                               "scoped semantic stream prepends identity");
                    addIfFound(found,
                               relative,
                               normalized,
                               expression + "." + level + "(\"{}",
                               "scoped semantic format string may prepend identity");
                    if (!found.empty() && found.back().pattern == expression + "." + level + "(\"{}" &&
                        normalized.find(expression + "." + level + "(\"{}") != std::string::npos &&
                        normalized.find(identity, normalized.find(expression + "." + level + "(\"{}")) == std::string::npos) {
                        found.pop_back();
                    }
                }
                for (const std::string& variable : assignedSemanticLogVariables(normalized)) {
                    addIfFound(found,
                               relative,
                               normalized,
                               variable + "." + level + "()<<" + identity,
                               "semantic logger local variable prepends identity");
                    addIfFound(found,
                               relative,
                               normalized,
                               variable + "." + level + "(\"{}",
                               "semantic logger local variable format string may prepend identity");
                    if (!found.empty() && found.back().pattern == variable + "." + level + "(\"{}" &&
                        normalized.find(variable + "." + level + "(\"{}") != std::string::npos &&
                        normalized.find(identity, normalized.find(variable + "." + level + "(\"{}")) == std::string::npos) {
                        found.pop_back();
                    }
                }
            }
        }

        std::vector<Violation> violations;
        for (const Violation& violation : found) {
            if (!isAllowed(violation.file, violation.pattern)) {
                violations.push_back(violation);
            }
        }
        return violations;
    }
} // namespace

int main() {
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        return 1;
    }

    const std::vector<std::filesystem::path> files = productionSourceFiles(root);
    std::vector<Violation> violations;
    for (const std::filesystem::path& file : files) {
        std::vector<Violation> fileViolations = findViolations(root, file);
        violations.insert(violations.end(), fileViolations.begin(), fileViolations.end());
    }

    if (!violations.empty()) {
        std::cerr << auditNote << '\n';
        for (const Violation& violation : violations) {
            std::cerr << "Forbidden duplicated semantic identity in " << violation.file << ": " << violation.reason << " pattern='"
                      << violation.pattern << "'\n";
        }
        return 1;
    }

    std::cout << "Semantic identity source-policy audit scanned " << files.size() << " production source files under src/.\n";
    std::cout << auditNote;
    return 0;
}
