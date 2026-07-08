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

    struct ForbiddenFragment {
        std::filesystem::path sourceFile;
        std::string fragment;
    };

} // namespace

int main() {
    const std::filesystem::path root = projectRoot();
    const std::vector<ForbiddenFragment> forbiddenFragments = {
        {root / "src" / "iot" / "mqtt" / "Mqtt.cpp", "mqttLog().info() << connectionName << \" MQTT: \" << packet.getName()"},
        {root / "src" / "iot" / "mqtt" / "Mqtt.cpp",
         "mqttLog().info() << connectionName << \" MQTT: ====================================\""},
        {root / "src" / "iot" / "mqtt" / "Mqtt.cpp", "mqttLog().info() << connectionName << \" MQTT: PUBLISH Resend\""},
        {root / "src" / "iot" / "mqtt" / "Mqtt.cpp", "mqttLog().info() << connectionName << \" MQTT: PUBREL Resend\""},
        {root / "src" / "iot" / "mqtt" / "Mqtt.cpp",
         "mqttLog().info() << connectionName << \" MQTT: \" << controlPacket.getName() << \" send"},
        {root / "src" / "iot" / "mqtt" / "Mqtt.cpp", "mqttLog().info() << connectionName << \" MQTT:   Topic: \""},
        {root / "src" / "iot" / "mqtt" / "Mqtt.cpp", "enabled(logger::LogLevel::Info)"},
        {root / "src" / "iot" / "mqtt" / "Mqtt.cpp",
         "mqttLog().info()\n                            << connectionName << \" MQTT:   Duplicate QoS2 PUBLISH suppressed"},
        {root / "src" / "iot" / "mqtt" / "Mqtt.cpp", "mqttLog().info() << connectionName << \" MQTT:   QoS2 PUBREL received"},
        {root / "src" / "iot" / "mqtt" / "Mqtt.cpp",
         "mqttLog().info()\n                    << connectionName << \" MQTT:   Duplicate QoS2 PUBREL"},
        {root / "src" / "web" / "http" / "server" / "SocketContext.cpp",
         "frameworkLog().info() << getSocketConnection()->getConnectionName() << \" HTTP: Request deliver"},
        {root / "src" / "web" / "http" / "server" / "SocketContext.cpp",
         "frameworkLog().info() << getSocketConnection()->getConnectionName() << \" HTTP: No more pending request"},
        {root / "src" / "web" / "http" / "server" / "SocketContext.cpp",
         "frameworkLog().info() << getSocketConnection()->getConnectionName()\n                                  << \" HTTP: Response "
         "start"},
        {root / "src" / "web" / "http" / "server" / "SocketContext.cpp",
         "frameworkLog().info() << getSocketConnection()->getConnectionName() << \" HTTP: Response completed for request"},
        {root / "src" / "web" / "http" / "server" / "SocketContext.cpp",
         "frameworkLog().info() << getSocketConnection()->getConnectionName() << \" HTTP: Connected"},
        {root / "src" / "web" / "http" / "client" / "SocketContext.cpp",
         "frameworkLog().info() << getSocketConnection()->getConnectionName() << \" HTTP: Request (\" << request->count"},
        {root / "src" / "web" / "http" / "client" / "SocketContext.cpp",
         "frameworkLog().info() << getSocketConnection()->getConnectionName() << \" HTTP: Response received"},
        {root / "src" / "web" / "http" / "client" / "SocketContext.cpp",
         "frameworkLog().info() << getSocketConnection()->getConnectionName() << \" HTTP: Request (\" << request->count\n                  "
         "            << \") completed"},
        {root / "src" / "web" / "http" / "client" / "SocketContext.cpp",
         "frameworkLog().info() << getSocketConnection()->getConnectionName() << \" HTTP: Connected"},
        {root / "src" / "web" / "http" / "client" / "SocketContext.cpp", "frameworkLog().warn() << httputils::toString("},
    };

    bool ok = true;
    for (const auto& forbiddenFragment : forbiddenFragments) {
        const std::string contents = readFile(forbiddenFragment.sourceFile);
        if (contents.empty()) {
            std::cerr << "Unable to read " << forbiddenFragment.sourceFile << '\n';
            ok = false;
            continue;
        }
        if (contains(contents, forbiddenFragment.fragment)) {
            std::cerr << "Forbidden high-frequency info logging fragment in " << forbiddenFragment.sourceFile << ": "
                      << forbiddenFragment.fragment << '\n';
            ok = false;
        }
    }

    return ok ? 0 : 1;
}
