#include "utils/SubCommand.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace {

    void require(bool condition, const std::string& message) {
        if (!condition) {
            std::cerr << message << std::endl;
            std::abort();
        }
    }

    class TestSubCommand : public utils::SubCommand {
    public:
        TestSubCommand(utils::SubCommand* parent, const std::string& name)
            : utils::SubCommand(parent, std::make_shared<utils::AppWithPtr>("test command", name, this), "Tests") {
        }
    };

} // namespace

int main() {
    TestSubCommand root{nullptr, "root"};
    const std::string secret = "s3cr3t-\\\"value";

    auto* secretOpt = root.addOption("--secret", "Sensitive value", "string", CLI::TypeValidator<std::string>());
    root.setConfigurable(secretOpt, true);
    root.setSensitive(secretOpt);
    root.setDefaultValue(secretOpt, secret);

    require(secretOpt->as<std::string>() == secret, "sensitive option no longer returns its real configured value");

    const std::string serialized = root.configToStr();
    require(serialized.find("s3cr3t") == std::string::npos, "config serialization exposed sensitive value");
    require(serialized.find("<REDACTED>") != std::string::npos, "config serialization did not mark sensitive value as redacted");

    root.setSensitive(secretOpt, false);
    const std::string unredacted = root.configToStr();
    require(unredacted.find("s3cr3t") != std::string::npos, "clearing sensitivity did not restore normal serialization");

    return EXIT_SUCCESS;
}
