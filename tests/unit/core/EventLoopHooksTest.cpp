#include "core/EventLoop.h"
#include "core/SNodeC.h"
#include "core/State.h"
#include "support/TestResult.h"
#include "utils/Config.h"

#include <csignal>
#include <sys/wait.h>
#include <unistd.h>

namespace {
    char applicationName[] = "event-loop-hooks-test";
    char* replacementArgv[] = {applicationName, nullptr};
    core::CommandLineArguments replacementArgs{1, replacementArgv};
    int initCalls = 0;
    int exitCalls = 0;
    int observedExitCode = -1;
    bool argumentsReceived = false;
    bool cleanupCompleted = false;
    bool signalRestored = false;
    struct sigaction originalIntAction{};

    bool rejectsInit(int argc, char* argv[]) {
        const pid_t child = fork();
        bool rejected = false;
        if (child == 0) {
            core::SNodeC::init(argc, argv);
            _exit(0);
        } else if (child > 0) {
            int status = 0;
            rejected = waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 1;
        }
        return rejected;
    }
} // namespace

namespace core {
    CommandLineArguments onInit(int argc, char* argv[]) {
        ++initCalls;
        argumentsReceived = argc > 0 && argv != nullptr;
        return replacementArgs;
    }

    int onExit(int exitCode) {
        ++exitCalls;
        observedExitCode = exitCode;
        cleanupCompleted = SNodeC::state() == State::STOPPING;
        struct sigaction currentIntAction{};
        signalRestored = sigaction(SIGINT, nullptr, &currentIntAction) == 0 && currentIntAction.sa_handler == originalIntAction.sa_handler;
        return 42;
    }
} // namespace core

int main(int argc, char* argv[]) {
    tests::support::TestResult result;
    int exitCode = tests::support::cTestSkipReturnCode;

    if (tests::support::shouldSkipRootWithoutSNodeCGroup()) {
        tests::support::printRootWithoutSNodeCGroupSkipMessage("EventLoopHooksTest");
    } else {
        const core::CommandLineArguments rejectedArguments[] = {{0, nullptr}, {0, replacementArgv}, {1, nullptr}, {-1, replacementArgv}};
        for (const auto& rejected : rejectedArguments) {
            replacementArgs = rejected;
            result.expectTrue(rejectsInit(argc, argv), "Invalid hook arguments reject initialization");
            result.expectTrue(core::SNodeC::state() == core::State::LOADED, "Rejected initialization preserves loaded state");
        }

        replacementArgs = {1, replacementArgv};
        core::SNodeC::init(argc, argv);
        result.expectTrue(argumentsReceived, "Strong onInit receives the original arguments");
        result.expectTrue(utils::Config::getApplicationName() == applicationName, "Configuration uses the hook's replacement arguments");
        result.expectTrue(core::SNodeC::state() == core::State::INITIALIZED, "Valid hook arguments allow initialization");

        replacementArgs = {0, nullptr};
        result.expectTrue(rejectsInit(argc, argv), "A rejected hook cannot report stale initialization success");
        result.expectEqual(1, initCalls, "The parent init call invokes the strong hook");
        result.expectEqual(0, exitCalls, "Initialization does not invoke onExit");
        result.expectEqual(0, sigaction(SIGINT, nullptr, &originalIntAction), "Capture original SIGINT action");

        result.expectEqual(42, core::SNodeC::start(), "Strong onExit replaces the final return code");
        result.expectEqual(0, observedExitCode, "onExit receives the normal framework exit code");
        result.expectEqual(1, exitCalls, "onExit runs once when start returns");
        result.expectTrue(cleanupCompleted, "onExit runs after event-loop shutdown");
        result.expectTrue(signalRestored, "onExit runs after restoring SIGINT");
        exitCode = result.processResult();
    }

    return exitCode;
}
