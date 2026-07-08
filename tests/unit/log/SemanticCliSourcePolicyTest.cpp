#include "tests/support/TestResult.h"
#include "tests/unit/log/SourcePolicyTestRoot.h"

#include <filesystem>
#include <fstream>
#include <string>

namespace {
    std::string readFile(const std::filesystem::path& path) {
        std::ifstream in(path);
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }

    bool contains(const std::string& text, const std::string& needle) {
        return text.find(needle) != std::string::npos;
    }
} // namespace

int main() {
    tests::support::TestResult result;
    const std::filesystem::path root = source_policy::sourcePolicyProjectRoot();

    const std::string configCpp = readFile(root / "src/utils/Config.cpp");
    result.expectTrue(contains(configCpp, "--log-format"), "root options include --log-format");
    result.expectTrue(contains(configCpp, "--log-origin-level"), "root options include --log-origin-level");
    result.expectTrue(contains(configCpp, "--log-boundary-level"), "root options include --log-boundary-level");
    result.expectTrue(contains(configCpp, "--log-component-level"), "root options include --log-component-level");
    result.expectTrue(contains(configCpp, "--log-instance-level"), "root options include --log-instance-level");
    result.expectTrue(contains(configCpp, "--log-level"), "root --log-level still exists");
    result.expectTrue(contains(configCpp, "--verbose-level"), "root --verbose-level still exists");
    result.expectTrue(contains(configCpp, "--quiet"), "root --quiet still exists");
    const std::string subCommandCpp = readFile(root / "src/utils/SubCommand.cpp");
    result.expectTrue(contains(subCommandCpp, "--log-file"), "root --log-file still exists");

    const std::string configInstanceCpp = readFile(root / "src/net/config/ConfigInstance.cpp");
    result.expectTrue(!contains(configInstanceCpp, "--log-level"), "ConfigInstance does not yet define its own --log-level in PR G.1");

    const std::string configSectionHpp = readFile(root / "src/net/config/ConfigSection.hpp");
    result.expectTrue(!contains(configSectionHpp, "--log-level") && !contains(configSectionHpp, "--log-component-level") &&
                          !contains(configSectionHpp, "--log-boundary-level") && !contains(configSectionHpp, "--log-origin-level"),
                      "ConfigSection does not define logging options");

    return result.processResult();
}
