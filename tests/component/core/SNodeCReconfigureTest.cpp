#include "core/EventReceiver.h"
#include "core/SNodeC.h"
#include "express/WebApp.h"
#include "log/SemanticLogger.h"
#include "net/config/ConfigInstance.h"
#include "support/TestResult.h"
#include "utils/Config.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <unistd.h>
#include <vector>

namespace {
    class Endpoint : public net::config::ConfigInstance {
    public:
        Endpoint(const std::string& name, Role role)
            : ConfigInstance(name, role)
            , valueOption(addOption("--value", "Runtime value", "INT", 7, CLI::TypeValidator<int>())) {
            finalCallback([this] {
                if (value() == rejectedValue)
                    throw CLI::ValidationError("--value", "rejected by endpoint final validation");
                ++validations;
            });
        }
        ~Endpoint() override = default;
        int value() const {
            return valueOption->as<int>();
        }
        int validations = 0;
        int rejectedValue = 13;

    private:
        CLI::Option* valueOption;
    };
} // namespace

int main(int, char* argv[]) {
    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("SNodeCReconfigureTest");
        return tests::support::cTestSkipReturnCode;
    }
    tests::support::TestResult result;
    result.expectTrue(!core::SNodeC::reconfigure(), "reconfiguration before init is rejected");
    char directoryTemplate[] = "/tmp/snodec-reconfigure-XXXXXX";
    const char* createdDirectory = ::mkdtemp(directoryTemplate);
    if (!createdDirectory)
        return EXIT_FAILURE;
    const std::filesystem::path directory(createdDirectory);
    const std::string configPath = (directory / "runtime.conf").string();
    const std::string forbiddenLog = (directory / "must-not-open.log").string();
    const auto writeConfig = [&](const std::string& value, bool startup = false) {
        std::ofstream config(configPath);
        config << "log-level=" << (startup ? "3" : "6") << "\nlog-format=" << (startup ? "json" : "text")
               << "\ndaemonize=" << (startup ? "false" : "true") << "\nenforce-log-file=" << (startup ? "false" : "true")
               << "\nlog-file=" << forbiddenLog << "\nruntime-value=" << (startup ? 10 : 20) << "\n[runtime-peer]\nvalue=" << value
               << "\n[cli-peer]\nvalue=22\n";
    };
    writeConfig("21", true);
    std::vector<std::string> arguments{argv[0], "--config-file", configPath, "cli-peer", "--value", "91"};
    std::vector<char*> argumentPointers;
    for (auto& argument : arguments)
        argumentPointers.push_back(argument.data());
    auto current = std::make_unique<Endpoint>("runtime-peer", Endpoint::Role::CLIENT);
    auto other = std::make_unique<Endpoint>("cli-peer", Endpoint::Role::SERVER);
    auto* runtimeValue = utils::Config::configRoot.addOption("--runtime-value", "Application setting", "INT", 0, CLI::TypeValidator<int>());
    core::SNodeC::init(static_cast<int>(argumentPointers.size()), argumentPointers.data());
    result.expectTrue(!core::SNodeC::reconfigure(), "startup configuration is left to start()");
    const pid_t processId = getpid();
    bool callbackRan = false;
    core::EventReceiver::atNextTick([&] {
        callbackRan = true;
        result.expectTrue(logger::LogManager::isFrozen(), "bootstrap freezes semantic policy");
        result.expectTrue(logger::LogManager::format() == logger::LogManager::Format::Json, "bootstrap applies the startup format");
        result.expectEqual(21, current->value(), "initial configuration reaches the client instance");
        result.expectEqual(91, other->value(), "command line overrides file configuration");
        const int previousValidations = other->validations;
        std::unique_ptr<Endpoint> replacement;
        bool recreated = false;
        writeConfig("42");
        current->setOnDestroy([&](auto*) {
            replacement = std::make_unique<Endpoint>("runtime-peer", Endpoint::Role::CLIENT);
            recreated = core::SNodeC::reconfigure();
        });
        current.reset();
        current = std::move(replacement);
        result.expectTrue(recreated, "same-name recreation can reconfigure inside the destruction callback");
        result.expectEqual(42, current->value(), "recreated client consumes the current config file");
        result.expectEqual(20, runtimeValue->as<int>(), "application root options are reapplied too");
        result.expectEqual(91, other->value(), "unreplaced server retains command-line precedence");
        result.expectEqual(previousValidations + 1, other->validations, "endpoint final callbacks remain repeatable");
        result.expectTrue(express::WebApp::reconfigure(), "Express forwards to the same runtime operation");
        result.expectTrue(core::SNodeC::reconfigure(), "repeated reconfiguration succeeds with a frozen logger");
        utils::Config::parse();
        result.expectTrue(logger::LogManager::isFrozen(), "legacy parsing does not unfreeze logging");
        result.expectTrue(logger::LogManager::format() == logger::LogManager::Format::Json,
                          "runtime parsing does not replace startup policy");
        const logger::LogScope scope{
            logger::LogOrigin::Application, logger::LogBoundary::Application, "reconfigure-test", {}, logger::LogRole::Unknown, {}};
        result.expectTrue(logger::LogManager::effectiveLevel(scope) == logger::LogLevel::Warn,
                          "changed log-level input does not change the frozen effective threshold");
        result.expectTrue(getpid() == processId, "runtime daemonize option does not fork the application");
        result.expectTrue(!std::filesystem::exists(forbiddenLog), "runtime parsing does not open a startup log file");
        std::filesystem::remove(configPath);
        result.expectTrue(!core::SNodeC::reconfigure(), "a missing explicit config file returns failure");
        writeConfig("not-an-integer");
        result.expectTrue(!core::SNodeC::reconfigure(), "invalid input returns failure without leaving the callback");
        writeConfig("43");
        other->rejectedValue = 91;
        result.expectTrue(!core::SNodeC::reconfigure(), "endpoint final validation errors return failure");
        other->rejectedValue = 13;
        result.expectTrue(core::SNodeC::reconfigure(), "corrected configuration can be applied after rejection");
        result.expectEqual(43, current->value(), "corrected value is applied");
        current.reset();
        other.reset();
        core::SNodeC::stop();
        result.expectTrue(!core::SNodeC::reconfigure(), "reconfiguration during shutdown is rejected");
    });
    result.expectEqual(0, core::SNodeC::start(), "runtime exits successfully");
    result.expectTrue(callbackRan, "the checks ran inside the event loop");
    result.expectTrue(!express::WebApp::reconfigure(), "reconfiguration after shutdown is rejected");
    std::filesystem::remove_all(directory);
    return result.processResult();
}
