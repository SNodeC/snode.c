#include "utils/SubCommand.h"

#include "net/config/ConfigInstance.h"
#include "utils/Config.h"

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

    class TestConfigInstance : public net::config::ConfigInstance {
    public:
        TestConfigInstance()
            : net::config::ConfigInstance("unit-instance", net::config::ConfigInstance::Role::SERVER) {
        }

        ~TestConfigInstance() override = default;
    };

} // namespace

int main() {
    utils::AppWithPtr app{"Synthetic app", "demo", nullptr};
    int requiredValue = 0;
    auto* requiredOpt = app.add_option("--required", requiredValue, "Required value");

    app.setCanonicalRequired(requiredOpt, true);
    app.setCanonicalNeed(requiredOpt, true);
    require(app.canonicalRequired(requiredOpt), "option canonical required state was not recorded");
    require(app.effectiveRequired(requiredOpt), "option effective required state was not applied initially");
    require(requiredOpt->get_required(), "CLI11 option required state was not applied initially");
    require(app.canonicalNeeds(requiredOpt), "option canonical need was not recorded");
    require(app.effectiveNeeds(requiredOpt), "option effective need was not applied initially");

    app.setSuppressionRecursive(utils::ConfigSuppressionReason::Disabled, true);
    require(app.canonicalRequired(requiredOpt), "disabled suppression erased option canonical required state");
    require(!app.effectiveRequired(requiredOpt), "disabled suppression did not clear option effective required state");
    require(!requiredOpt->get_required(), "disabled suppression did not clear CLI11 option required state");
    require(app.canonicalNeeds(requiredOpt), "disabled suppression erased option canonical need");
    require(!app.effectiveNeeds(requiredOpt), "disabled suppression did not clear option effective need");

    app.setSuppressionRecursive(utils::ConfigSuppressionReason::Disabled, false);
    require(app.canonicalRequired(requiredOpt), "re-enable erased option canonical required state");
    require(app.effectiveRequired(requiredOpt), "re-enable did not restore option effective required state");
    require(requiredOpt->get_required(), "re-enable did not restore CLI11 option required state");
    require(app.effectiveNeeds(requiredOpt), "re-enable did not restore option effective need");

    app.setSuppressionRecursive(utils::ConfigSuppressionReason::Disabled, true);
    app.setSuppressionRecursive(utils::ConfigSuppressionReason::ForceUnrequired, true);
    app.setSuppressionRecursive(utils::ConfigSuppressionReason::Disabled, false);
    require(app.canonicalRequired(requiredOpt), "multiple suppressions erased canonical option required state");
    require(!app.effectiveRequired(requiredOpt), "removing one of two suppressions restored option effective required state too early");
    require(!requiredOpt->get_required(), "removing one of two suppressions restored CLI11 option required state too early");
    app.setSuppressionRecursive(utils::ConfigSuppressionReason::ForceUnrequired, false);
    require(app.effectiveRequired(requiredOpt), "removing final suppression did not restore option effective required state");
    require(requiredOpt->get_required(), "removing final suppression did not restore CLI11 option required state");

    utils::AppWithPtr parent{"Parent app", "parent", nullptr};
    auto appChild = std::make_shared<utils::AppWithPtr>("Child section", "section", nullptr);
    parent.add_subcommand(appChild);
    parent.setCanonicalNeed(appChild.get(), true);
    appChild->setCanonicalRequired(true);
    require(appChild->canonicalRequired(), "child canonical required state was not recorded");
    require(appChild->effectiveRequired(), "child effective required state was not applied initially");
    require(appChild->get_required(), "CLI11 child required state was not applied initially");
    require(parent.canonicalNeeds(appChild.get()), "child canonical need was not recorded");
    require(parent.effectiveNeeds(appChild.get()), "child effective need was not applied initially");

    parent.setSuppressionRecursive(utils::ConfigSuppressionReason::Disabled, true);
    require(appChild->canonicalRequired(), "disabled suppression erased child canonical required state");
    require(!appChild->effectiveRequired(), "disabled suppression did not clear child effective required state");
    require(!appChild->get_required(), "disabled suppression did not clear CLI11 child required state");
    require(parent.canonicalNeeds(appChild.get()), "disabled suppression erased child canonical need");
    require(!parent.effectiveNeeds(appChild.get()), "disabled suppression did not clear child effective need");

    parent.setSuppressionRecursive(utils::ConfigSuppressionReason::Disabled, false);
    require(appChild->canonicalRequired(), "re-enable erased child canonical required state");
    require(appChild->effectiveRequired(), "re-enable did not restore child effective required state");
    require(appChild->get_required(), "re-enable did not restore CLI11 child required state");
    require(parent.effectiveNeeds(appChild.get()), "re-enable did not restore child effective need");

    std::string defaulted = "from-api";
    auto* defaultedOpt = app.add_option("--defaulted", defaulted, "Defaulted required value")->default_str(defaulted);
    app.setCanonicalRequired(defaultedOpt, true);
    require(app.canonicalRequired(defaultedOpt), "C++ default erased canonical required state");
    require(app.effectiveRequired(defaultedOpt), "C++ default erased effective required state");

    app.setSuppressionRecursive(utils::ConfigSuppressionReason::Disabled, true);
    require(!requiredOpt->get_required(), "disabled owner still requires nested option");
    app.setSuppressionRecursive(utils::ConfigSuppressionReason::Disabled, false);
    require(requiredOpt->get_required(), "re-enabled owner left stale non-required CLI11 state");

    TestSubCommand root{nullptr, "root"};
    TestSubCommand child{&root, "child"};

    child.required(true);
    require(child.getAppWithPtr()->canonicalRequired(), "SubCommand child canonical required state was not recorded");
    require(child.getAppWithPtr()->effectiveRequired(), "SubCommand child effective required state was not applied");
    require(root.getAppWithPtr()->canonicalNeeds(child.getAppWithPtr()), "SubCommand parent canonical need was not recorded");
    require(root.getAppWithPtr()->effectiveNeeds(child.getAppWithPtr()), "SubCommand parent effective need was not applied");
    require(root.getAppWithPtr()->effectiveRequired(), "SubCommand parent effective required state did not reflect child requirement");

    child.forceUnrequired(true);
    require(child.getAppWithPtr()->canonicalRequired(), "forceUnrequired erased SubCommand child canonical required state");
    require(!child.getAppWithPtr()->effectiveRequired(), "forceUnrequired did not suppress SubCommand child effective required state");
    require(root.getAppWithPtr()->canonicalNeeds(child.getAppWithPtr()), "forceUnrequired erased SubCommand parent canonical need");
    require(!root.getAppWithPtr()->effectiveNeeds(child.getAppWithPtr()), "forceUnrequired did not suppress SubCommand parent effective need");
    require(!root.getAppWithPtr()->effectiveRequired(), "forceUnrequired left stale SubCommand parent effective required state");

    child.forceUnrequired(false);
    require(child.getAppWithPtr()->canonicalRequired(), "forceUnrequired(false) erased SubCommand child canonical required state");
    require(child.getAppWithPtr()->effectiveRequired(), "forceUnrequired(false) did not restore SubCommand child effective required state");
    require(root.getAppWithPtr()->canonicalNeeds(child.getAppWithPtr()), "forceUnrequired(false) erased SubCommand parent canonical need");
    require(root.getAppWithPtr()->effectiveNeeds(child.getAppWithPtr()), "forceUnrequired(false) did not restore SubCommand parent effective need");
    require(root.getAppWithPtr()->effectiveRequired(), "forceUnrequired(false) did not restore SubCommand parent effective required state");

    auto* childOption = child.addOption("--child-option", "Child option", "INT", CLI::NonNegativeNumber);
    child.required(childOption, true);
    require(child.getAppWithPtr()->canonicalRequired(childOption), "SubCommand option canonical required state was not recorded");
    require(child.getAppWithPtr()->effectiveRequired(childOption), "SubCommand option effective required state was not applied");
    require(childOption->get_required(), "CLI11 SubCommand option required state was not applied");
    child.forceUnrequired(true);
    require(child.getAppWithPtr()->canonicalRequired(childOption), "forceUnrequired erased SubCommand option canonical required state");
    require(!child.getAppWithPtr()->effectiveRequired(childOption), "forceUnrequired did not suppress SubCommand option effective required state");
    require(!childOption->get_required(), "forceUnrequired did not suppress CLI11 option required state");
    child.forceUnrequired(false);
    require(child.getAppWithPtr()->effectiveRequired(childOption), "forceUnrequired(false) did not restore SubCommand option effective required state");
    require(childOption->get_required(), "forceUnrequired(false) did not restore CLI11 option required state");

    child.getAppWithPtr()->setSuppressionRecursive(utils::ConfigSuppressionReason::Disabled, true);
    child.forceUnrequired(true);
    child.getAppWithPtr()->setSuppressionRecursive(utils::ConfigSuppressionReason::Disabled, false);
    require(!child.getAppWithPtr()->effectiveRequired(), "mixed SubCommand suppressions restored effective required state too early");
    child.forceUnrequired(false);
    require(child.getAppWithPtr()->effectiveRequired(), "mixed SubCommand suppressions did not restore effective required state");

    TestSubCommand suppressedRoot{nullptr, "suppressed-root"};
    TestSubCommand suppressedChild{&suppressedRoot, "suppressed-child"};
    suppressedChild.forceUnrequired(true);
    suppressedChild.required(true);
    require(suppressedChild.getAppWithPtr()->canonicalRequired(), "required while suppressed did not record child canonical required");
    require(!suppressedChild.getAppWithPtr()->effectiveRequired(), "required while suppressed restored child effective required too early");
    require(suppressedRoot.getAppWithPtr()->canonicalNeeds(suppressedChild.getAppWithPtr()),
            "required while suppressed did not record parent canonical need");
    require(!suppressedRoot.getAppWithPtr()->effectiveNeeds(suppressedChild.getAppWithPtr()),
            "required while suppressed applied parent effective need too early");
    require(suppressedRoot.getAppWithPtr()->canonicalRequired(), "required while suppressed did not record parent canonical contribution");
    require(!suppressedRoot.getAppWithPtr()->effectiveRequired(), "required while suppressed applied parent effective contribution too early");

    TestSubCommand removeRoot{nullptr, "remove-root"};
    TestSubCommand removeChild{&removeRoot, "remove-child"};
    removeChild.required(true);
    removeChild.forceUnrequired(true);
    removeChild.required(false);
    require(!removeChild.getAppWithPtr()->canonicalRequired(), "required(false) while suppressed left child canonical required");
    require(!removeChild.getAppWithPtr()->effectiveRequired(), "required(false) while suppressed left child effective required");
    require(!removeRoot.getAppWithPtr()->canonicalNeeds(removeChild.getAppWithPtr()),
            "required(false) while suppressed left parent canonical need");
    require(!removeRoot.getAppWithPtr()->effectiveNeeds(removeChild.getAppWithPtr()),
            "required(false) while suppressed left parent effective need");
    require(!removeRoot.getAppWithPtr()->canonicalRequired(), "required(false) while suppressed left parent canonical contribution");
    require(!removeRoot.getAppWithPtr()->effectiveRequired(), "required(false) while suppressed decremented/restored parent effective contribution incorrectly");
    removeChild.forceUnrequired(false);
    require(!removeChild.getAppWithPtr()->canonicalRequired(), "restore after suppressed removal changed child canonical required");
    require(!removeChild.getAppWithPtr()->effectiveRequired(), "restore after suppressed removal changed child effective required");
    require(!removeRoot.getAppWithPtr()->canonicalNeeds(removeChild.getAppWithPtr()),
            "restore after suppressed removal restored parent canonical need");
    require(!removeRoot.getAppWithPtr()->effectiveNeeds(removeChild.getAppWithPtr()),
            "restore after suppressed removal restored parent effective need");
    require(!removeRoot.getAppWithPtr()->effectiveRequired(), "restore after suppressed removal corrupted parent effective count");

    TestSubCommand countedRoot{nullptr, "counted-root"};
    TestSubCommand countedChild{&countedRoot, "counted-child"};
    countedChild.required(true);
    countedChild.required(true);
    countedChild.forceUnrequired(true);
    countedChild.required(false);
    require(countedChild.getAppWithPtr()->canonicalRequired(), "balanced counted sequence removed child canonical required too early");
    require(!countedChild.getAppWithPtr()->effectiveRequired(), "balanced counted sequence restored child effective while still suppressed");
    require(countedRoot.getAppWithPtr()->canonicalNeeds(countedChild.getAppWithPtr()),
            "balanced counted sequence removed parent canonical need too early");
    require(!countedRoot.getAppWithPtr()->effectiveNeeds(countedChild.getAppWithPtr()),
            "balanced counted sequence restored parent effective need while child still suppressed");
    countedChild.forceUnrequired(false);
    require(countedChild.getAppWithPtr()->effectiveRequired(), "balanced counted sequence did not restore child effective required");
    require(countedRoot.getAppWithPtr()->effectiveRequired(), "balanced counted sequence did not restore parent effective required");
    countedChild.required(false);
    require(!countedChild.getAppWithPtr()->canonicalRequired(), "balanced counted sequence left child canonical required");
    require(!countedChild.getAppWithPtr()->effectiveRequired(), "balanced counted sequence left child effective required");
    require(!countedRoot.getAppWithPtr()->canonicalNeeds(countedChild.getAppWithPtr()),
            "balanced counted sequence left parent canonical need");
    require(!countedRoot.getAppWithPtr()->effectiveNeeds(countedChild.getAppWithPtr()),
            "balanced counted sequence left parent effective need");
    require(!countedRoot.getAppWithPtr()->effectiveRequired(), "balanced counted sequence left parent effective required");

    TestConfigInstance instance;
    instance.required(true);
    require(utils::Config::configRoot.getAppWithPtr()->canonicalNeeds(instance.getAppWithPtr()),
            "ConfigInstance parent canonical need was not recorded");
    require(utils::Config::configRoot.getAppWithPtr()->effectiveNeeds(instance.getAppWithPtr()),
            "ConfigInstance parent effective need was not applied");
    instance.setDisabled(true);
    require(instance.getAppWithPtr()->canonicalRequired(), "ConfigInstance disable erased canonical required state");
    require(!instance.getAppWithPtr()->effectiveRequired(), "ConfigInstance disable did not suppress effective required state");
    require(utils::Config::configRoot.getAppWithPtr()->canonicalNeeds(instance.getAppWithPtr()),
            "ConfigInstance disable erased parent canonical need");
    require(!utils::Config::configRoot.getAppWithPtr()->effectiveNeeds(instance.getAppWithPtr()),
            "ConfigInstance disable did not suppress parent effective need");
    instance.setDisabled(false);
    require(instance.getAppWithPtr()->canonicalRequired(), "ConfigInstance re-enable erased canonical required state");
    require(instance.getAppWithPtr()->effectiveRequired(), "ConfigInstance re-enable did not restore effective required state");
    require(utils::Config::configRoot.getAppWithPtr()->effectiveNeeds(instance.getAppWithPtr()),
            "ConfigInstance re-enable did not restore parent effective need");

    return 0;
}
