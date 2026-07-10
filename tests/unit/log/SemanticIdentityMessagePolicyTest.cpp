#include "SourcePolicyTestRoot.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {
    std::string compact(std::string_view value) {
        std::string result;
        result.reserve(value.size());
        for (const unsigned char ch : value) {
            if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') {
                result.push_back(static_cast<char>(ch));
            }
        }
        return result;
    }

    struct SourcePolicyFile {
        std::filesystem::path path;
        bool scopedConnectionLogger;
        bool scopedInstanceLogger;
    };

    struct ForbiddenPattern {
        std::string pattern;
        std::string description;
    };
} // namespace

int main() {
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        return 1;
    }

    const std::vector<SourcePolicyFile> files = {
        {root / "src" / "core" / "socket" / "stream" / "SocketContext.cpp", true, true},
        {root / "src" / "web" / "http" / "server" / "SocketContext.cpp", true, true},
        {root / "src" / "web" / "http" / "client" / "SocketContext.cpp", true, true},
    };

    const std::vector<std::string> levels = {"trace", "debug", "info", "warn", "error", "critical"};

    bool ok = true;
    for (const auto& file : files) {
        const std::string contents = source_policy::readSourcePolicyFile(file.path);
        if (contents.empty()) {
            std::cerr << "Unable to read " << file.path << '\n';
            ok = false;
            continue;
        }

        const std::string normalized = compact(contents);
        std::vector<ForbiddenPattern> patterns;
        for (const std::string& level : levels) {
            if (file.scopedConnectionLogger) {
                patterns.push_back({"frameworkLog()." + level + "()<<getSocketConnection()->getConnectionName()",
                                    "frameworkLog scope already carries connection identity"});
                patterns.push_back({"frameworkLog()." + level + "()<<socketContext->getSocketConnection()->getConnectionName()",
                                    "frameworkLog scope already carries connection identity"});
                patterns.push_back({"log()." + level + "()<<getConnectionName()", "log scope already carries connection identity"});
            }
            if (file.scopedInstanceLogger) {
                patterns.push_back({"frameworkLog()." + level + "()<<getSocketConnection()->getInstanceName()",
                                    "frameworkLog scope already carries instance identity"});
                patterns.push_back({"log()." + level + "()<<getInstanceName()", "log scope already carries instance identity"});
            }
        }

        for (const auto& pattern : patterns) {
            if (source_policy::contains(normalized, pattern.pattern)) {
                std::cerr << "Forbidden duplicated semantic identity in " << file.path << ": " << pattern.description << " pattern='"
                          << pattern.pattern << "'\n";
                ok = false;
            }
        }
    }

    return ok ? 0 : 1;
}
