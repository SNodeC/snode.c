#include "SourcePolicyTestRoot.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

int main() {
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        return 1;
    }
    const std::vector<std::filesystem::path> sourceFiles = {
        root / "src" / "iot" / "mqtt" / "server" / "Mqtt.cpp",
        root / "src" / "apps" / "oauth2" / "authorization_server" / "AuthorizationServer.cpp",
        root / "src" / "apps" / "oauth2" / "resource_server" / "ResourceServer.cpp",
        root / "src" / "apps" / "oauth2" / "client_app" / "ClientApp.cpp",
    };
    const std::vector<std::string> forbiddenLogFragments = {
        "WillMessage: \" << connect.getWillMessage()",
        "Password: \" << connect.getPassword()",
        "Username: \" << connect.getUsername()",
        "RefreshToken: \" <<",
        "AccessToken: \" <<",
        "Invalid access token '\" <<",
        "Valid access token '\" <<",
        "Recieving auth code",
        "Receiving auth code",
        "req.query(\"code\")",
        "authorization code value",
    };

    bool ok = true;
    for (const auto& sourceFile : sourceFiles) {
        const std::string contents = source_policy::readSourcePolicyFile(sourceFile);
        if (contents.empty()) {
            std::cerr << "Unable to read " << sourceFile << '\n';
            ok = false;
            continue;
        }
        for (const auto& fragment : forbiddenLogFragments) {
            if (source_policy::contains(contents, fragment)) {
                std::cerr << "Forbidden sensitive logging fragment in " << sourceFile << ": " << fragment << '\n';
                ok = false;
            }
        }
    }

    return ok ? 0 : 1;
}
