#include "net/config/ConfigInstance.h"
#include "utils/Config.h"
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
            : SubCommand(parent, std::make_shared<utils::AppWithPtr>("test command", name, this), "Tests") {
        }
    };

    class TestConfigInstance : public net::config::ConfigInstance {
    public:
        TestConfigInstance()
            : ConfigInstance("unit-instance", Role::SERVER) {
        }
    };

    bool parseAccepted(utils::SubCommand& command, const std::string& arguments) {
        bool accepted = false;
        try {
            command.getAppWithPtr()->parse(arguments);
            accepted = true;
        } catch (const CLI::ParseError&) {
        }

        return accepted;
    }

} // namespace

int main() {
    TestSubCommand root{nullptr, "root"};
    TestSubCommand child{&root, "child"};
    root.addOption("--probe", "Probe", "INT", CLI::TypeValidator<int>());
    auto* value = child.addOption("--value", "Required value", "INT", CLI::TypeValidator<int>());
    child.required(value);
    require(child.getRequired() && root.getRequired() && value->get_required(), "required option did not propagate");
    require(!parseAccepted(root, "--probe 1"), "missing required child was accepted");
    require(parseAccepted(root, "--probe 1 child --value 5"), "populated required child was rejected");

    child.forceUnrequired(true);
    require(!child.getRequired() && !root.getRequired() && !value->get_required(), "suppression left effective requirements active");
    require(parseAccepted(root, "--probe 1"), "suppressed requirements blocked parsing");
    child.forceUnrequired(false);
    require(child.getRequired() && root.getRequired() && value->get_required(), "restoration lost declared requirements");
    require(!parseAccepted(root, "--probe 1"), "restored requirements were not enforced");

    TestSubCommand grandchild{&child, "grandchild"};
    auto* nested = grandchild.addOption("--nested", "Nested required value", "INT", CLI::TypeValidator<int>());
    grandchild.required(nested);
    child.forceUnrequired(true);
    require(!nested->get_required() && !grandchild.getRequired(), "parent suppression missed descendant requirements");
    child.forceUnrequired(false);
    require(nested->get_required() && grandchild.getRequired(), "parent restoration lost descendant requirements");
    require(!parseAccepted(root, "--probe 1 child --value 5"), "missing nested requirement was accepted");
    require(parseAccepted(root, "--probe 1 child --value 5 grandchild --nested 6"), "populated nested requirements were rejected");
    grandchild.required(nested, false);
    child.setDefaultValue(value, 9);
    child.required(value, false);
    require(!value->get_required() && value->get_default_str() == "9", "API default or optional declaration was lost");
    require(parseAccepted(root, "--probe 1"), "removed requirements still blocked parsing");
    require(value->as<int>() == 9, "API default was not effective");

    TestSubCommand countedRoot{nullptr, "counted-root"};
    TestSubCommand countedChild{&countedRoot, "counted-child"};
    countedChild.required(true);
    countedChild.required(true);
    countedChild.forceUnrequired(true);
    countedChild.required(false);
    require(!countedChild.getRequired() && !countedRoot.getRequired(), "counted decrement restored a suppressed requirement");
    countedChild.forceUnrequired(false);
    require(countedChild.getRequired() && countedRoot.getRequired(), "counted requirement was removed too early");
    countedChild.required(false);
    require(!countedChild.getRequired() && !countedRoot.getRequired(), "balanced counted requirements remained active");

    countedChild.required(true);
    countedChild.forceUnrequired(true);
    countedRoot.required(false);
    countedChild.forceUnrequired(false);
    require(!countedRoot.getRequired() && countedChild.getRequired(), "existing parent counted-removal behavior changed");

    TestSubCommand suppressedRoot{nullptr, "suppressed-root"};
    TestSubCommand suppressedChild{&suppressedRoot, "suppressed-child"};
    suppressedChild.forceUnrequired(true);
    suppressedChild.required(true);
    require(!suppressedChild.getRequired() && !suppressedRoot.getRequired(), "declaration bypassed active suppression");
    suppressedChild.forceUnrequired(false);
    require(suppressedChild.getRequired() && suppressedRoot.getRequired(), "suppressed declaration was lost");
    suppressedChild.forceUnrequired(true);
    suppressedChild.required(false);
    suppressedChild.forceUnrequired(false);
    require(!suppressedChild.getRequired() && !suppressedRoot.getRequired(), "removed declaration returned after restoration");

    TestSubCommand independentRoot{nullptr, "independent-root"};
    TestSubCommand first{&independentRoot, "first"};
    TestSubCommand second{&independentRoot, "second"};
    first.required(true);
    second.required(true);
    first.forceUnrequired(true);
    require(independentRoot.getRequired() && second.getRequired(), "suppressing one child removed an independent requirement");
    second.forceUnrequired(true);
    require(!independentRoot.getRequired(), "root stayed required after all contributions were suppressed");
    first.forceUnrequired(false);
    require(independentRoot.getRequired(), "restoring one contribution did not restore root requiredness");

    TestConfigInstance instance;
    auto* instanceValue = instance.addOption("--required-value", "Instance required value", "INT", CLI::TypeValidator<int>());
    instance.required(instanceValue);
    instance.setDisabled(true);
    require(!instance.getRequired() && !instanceValue->get_required(), "setDisabled did not suppress requirements");
    instance.forceUnrequired(true);
    instance.setDisabled(false);
    require(!instance.getRequired() && !instanceValue->get_required(), "removing one suppression removed another");
    instance.forceUnrequired(false);
    require(instance.getRequired() && instanceValue->get_required(), "removing final suppression lost requirements");
    require(utils::Config::configRoot.getRequired(), "instance requirement did not propagate to config root");
    require(!parseAccepted(utils::Config::configRoot, "unit-instance --disabled=false"), "enabled instance accepted a missing requirement");
    require(parseAccepted(utils::Config::configRoot, "unit-instance --disabled"), "disabled instance required its value");
    require(instance.getDisabled() && !instanceValue->get_required(), "disabled CLI flag did not suppress requirements");
    require(parseAccepted(utils::Config::configRoot, "unit-instance --disabled=false --required-value 17"),
            "re-enabled populated instance was rejected");
    require(!instance.getDisabled() && instanceValue->get_required() && instanceValue->as<int>() == 17,
            "CLI re-enable did not restore behavior");

    return 0;
}
