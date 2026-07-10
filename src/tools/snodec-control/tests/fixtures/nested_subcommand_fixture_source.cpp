// Throwaway fixture program: registers genuinely nested, non-Instances/Sections user-defined
// SubCommands (plus a top-level custom subcommand alongside an Instances-style node, a custom
// SubCommand nested *inside* an Instances-style node, and a disabled subcommand), then relies on
// -s output being captured externally (run with `--show-config`). Not part of any build target;
// built and run manually to obtain a real fixture for snodec-control's tests.
#include "utils/Config.h"
#include "utils/SubCommand.h"

#include <string>
#include <string_view>

class Inner : public utils::SubCommand {
public:
    constexpr static std::string_view NAME{"inner"};
    constexpr static std::string_view DESCRIPTION{"Innermost custom node"};

    explicit Inner(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Deep") {
        addOption("--depth-value", "A deeply nested option", "value", std::string("deep"), CLI::Validator());
    }
};

class Outer : public utils::SubCommand {
public:
    constexpr static std::string_view NAME{"outer"};
    constexpr static std::string_view DESCRIPTION{"Outer custom node containing another custom node"};

    explicit Outer(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Custom") {
        addOption("--outer-value", "An outer option", "value", std::string("outer"), CLI::Validator());
        newSubCommand<Inner>();
    }
};

class TopLevelTool : public utils::SubCommand {
public:
    constexpr static std::string_view NAME{"tool"};
    constexpr static std::string_view DESCRIPTION{"Top-level custom subcommand alongside an Instances-style node"};

    explicit TopLevelTool(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Tools") {
        addOption("--tool-value", "A top-level tool option", "value", std::string("tool"), CLI::Validator());
    }
};

// Stands in for a real net::config::ConfigInstance (which requires the full network stack to
// instantiate) purely to prove out group == "Instances" classification with a nested custom
// SubCommand inside it; group is passed exactly as ConfigInstance itself would pass it.
class FakeInstance : public utils::SubCommand {
public:
    constexpr static std::string_view NAME{"echoserver"};
    constexpr static std::string_view DESCRIPTION{"A stand-in instance node"};

    explicit FakeInstance(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Instances") {
        addOption("--enabled", "Whether the instance is enabled", "BOOL", std::string("true"), CLI::IsMember({"true", "false"}));
        newSubCommand<Outer>();
    }
};

class LegacyDisabled : public utils::SubCommand {
public:
    constexpr static std::string_view NAME{"legacy"};
    constexpr static std::string_view DESCRIPTION{"A disabled custom node"};

    explicit LegacyDisabled(utils::SubCommand* parent)
        : utils::SubCommand(parent, this, "Custom") {
        addOption("--legacy-value", "An option under a disabled node", "value", std::string("legacy"), CLI::Validator());
        getAppWithPtr()->disabled();
    }
};

int main(int argc, char* argv[]) {
    utils::Config::configRoot.newSubCommand<TopLevelTool>();
    utils::Config::configRoot.newSubCommand<FakeInstance>();
    utils::Config::configRoot.newSubCommand<LegacyDisabled>();

    if (!utils::Config::init(argc, argv)) {
        return 1;
    }
    utils::Config::bootstrap();
    utils::Config::terminate();

    return 0;
}
