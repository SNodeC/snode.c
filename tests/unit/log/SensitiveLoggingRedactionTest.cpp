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

} // namespace

int main() {
    const std::filesystem::path root = projectRoot();
    const std::vector<std::filesystem::path> sourceFiles = {
        root / "src" / "iot" / "mqtt" / "server" / "Mqtt.cpp",
        root / "src" / "apps" / "oauth2" / "authorization_server" / "AuthorizationServer.cpp",
        root / "src" / "apps" / "oauth2" / "resource_server" / "ResourceServer.cpp",
    };
    const std::vector<std::string> forbiddenLogFragments = {
        "WillMessage: \" << connect.getWillMessage()",
        "Password: \" << connect.getPassword()",
        "Username: \" << connect.getUsername()",
        "RefreshToken: \" <<",
        "AccessToken: \" <<",
        "Invalid access token '\" <<",
        "Valid access token '\" <<",
    };

    bool ok = true;
    for (const auto& sourceFile : sourceFiles) {
        const std::string contents = readFile(sourceFile);
        if (contents.empty()) {
            std::cerr << "Unable to read " << sourceFile << '\n';
            ok = false;
            continue;
        }
        for (const auto& fragment : forbiddenLogFragments) {
            if (contains(contents, fragment)) {
                std::cerr << "Forbidden sensitive logging fragment in " << sourceFile << ": " << fragment << '\n';
                ok = false;
            }
        }
    }

    return ok ? 0 : 1;
}
