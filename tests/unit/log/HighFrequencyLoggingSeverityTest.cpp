#include "SourcePolicyTestRoot.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace {

    struct ForbiddenFragment {
        std::filesystem::path sourceFile;
        std::string fragment;
    };

} // namespace

int main() {
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();
    if (root.empty()) {
        return 1;
    }
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
        const std::string contents = source_policy::readSourcePolicyFile(forbiddenFragment.sourceFile);
        if (contents.empty()) {
            std::cerr << "Unable to read " << forbiddenFragment.sourceFile << '\n';
            ok = false;
            continue;
        }
        if (source_policy::contains(contents, forbiddenFragment.fragment)) {
            std::cerr << "Forbidden high-frequency info logging fragment in " << forbiddenFragment.sourceFile << ": "
                      << forbiddenFragment.fragment << '\n';
            ok = false;
        }
    }

    return ok ? 0 : 1;
}
