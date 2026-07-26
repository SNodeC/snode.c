// Compile the installed public header twice in one otherwise isolated
// translation unit to prove conventional include-guard behavior.
// clang-format off
#include <ai/openai/codex/typed/Commands.h>
#include <ai/openai/codex/typed/Commands.h>
// clang-format on

#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

int main() {
    namespace typed = ai::openai::codex::typed;

    using Exec = typed::Commands::Submission (typed::Commands::*)(typed::CommandExecParams, typed::Commands::ExecResultHandler);
    using Resize = typed::Commands::Submission (typed::Commands::*)(typed::CommandExecResizeParams, typed::Commands::UnitResultHandler);
    using Terminate =
        typed::Commands::Submission (typed::Commands::*)(typed::CommandExecTerminateParams, typed::Commands::UnitResultHandler);
    using Write = typed::Commands::Submission (typed::Commands::*)(typed::CommandExecWriteParams, typed::Commands::UnitResultHandler);

    static_assert(std::is_same_v<decltype(&typed::Commands::exec), Exec>);
    static_assert(std::is_same_v<decltype(&typed::Commands::resize), Resize>);
    static_assert(std::is_same_v<decltype(&typed::Commands::terminate), Terminate>);
    static_assert(std::is_same_v<decltype(&typed::Commands::write), Write>);
    static_assert(std::is_same_v<decltype(typed::CommandExecTerminalSize::cols), std::uint16_t>);
    static_assert(std::is_same_v<decltype(typed::CommandExecResponse::exitCode), std::int32_t>);

    const typed::CommandExecParams params{
        .command = {"synthetic-command", "argument with spaces", ""},
        .cwd = typed::OptionalNullable<std::string>::explicitNull(),
        .disableOutputCap = false,
        .disableTimeout = true,
        .env = typed::OptionalNullable<std::map<std::string, std::optional<std::string>>>::withValue(
            {{"SET", "synthetic-value"}, {"UNSET", std::nullopt}}),
        .outputBytesCap = typed::OptionalNullable<std::uint64_t>::withValue(std::numeric_limits<std::uint64_t>::max()),
        .processId = typed::OptionalNullable<typed::CommandExecProcessId>::withValue({"installed-process"}),
        .sandboxPolicy = typed::OptionalNullable<typed::SandboxPolicy>::explicitNull(),
        .size = typed::OptionalNullable<typed::CommandExecTerminalSize>::withValue({120, 40}),
        .streamStdin = true,
        .streamStdoutStderr = true,
        .timeoutMs = typed::OptionalNullable<std::int64_t>::withValue(30'000),
        .tty = true,
    };
    const typed::CommandExecWriteParams write{
        .processId = {"installed-process"},
        .deltaBase64 = typed::OptionalNullable<std::string>::withValue("c3ludGhldGlj"),
        .closeStdin = true,
    };
    const typed::CommandExecOutputDeltaNotification output{
        .capReached = false,
        .deltaBase64 = "c3ludGhldGlj",
        .processId = {"installed-process"},
        .stream = typed::CommandExecOutputStream::stdoutStream(),
    };
    [[maybe_unused]] const typed::OperationResult<typed::CommandExecResponse> response;

    return params.command.size() == 3 && params.cwd.isNull() && write.deltaBase64.hasValue() && output.stream.isKnown() ? 0 : 1;
}
