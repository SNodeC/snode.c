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
    auto child = std::make_shared<utils::AppWithPtr>("Child section", "section", nullptr);
    parent.add_subcommand(child);
    parent.setCanonicalNeed(child.get(), true);
    child->setCanonicalRequired(true);
    require(child->canonicalRequired(), "child canonical required state was not recorded");
    require(child->effectiveRequired(), "child effective required state was not applied initially");
    require(child->get_required(), "CLI11 child required state was not applied initially");
    require(parent.canonicalNeeds(child.get()), "child canonical need was not recorded");
    require(parent.effectiveNeeds(child.get()), "child effective need was not applied initially");

    parent.setSuppressionRecursive(utils::ConfigSuppressionReason::Disabled, true);
    require(child->canonicalRequired(), "disabled suppression erased child canonical required state");
    require(!child->effectiveRequired(), "disabled suppression did not clear child effective required state");
    require(!child->get_required(), "disabled suppression did not clear CLI11 child required state");
    require(parent.canonicalNeeds(child.get()), "disabled suppression erased child canonical need");
    require(!parent.effectiveNeeds(child.get()), "disabled suppression did not clear child effective need");

    parent.setSuppressionRecursive(utils::ConfigSuppressionReason::Disabled, false);
    require(child->canonicalRequired(), "re-enable erased child canonical required state");
    require(child->effectiveRequired(), "re-enable did not restore child effective required state");
    require(child->get_required(), "re-enable did not restore CLI11 child required state");
    require(parent.effectiveNeeds(child.get()), "re-enable did not restore child effective need");

    std::string defaulted = "from-api";
    auto* defaultedOpt = app.add_option("--defaulted", defaulted, "Defaulted required value")->default_str(defaulted);
    app.setCanonicalRequired(defaultedOpt, true);
    require(app.canonicalRequired(defaultedOpt), "C++ default erased canonical required state");
    require(app.effectiveRequired(defaultedOpt), "C++ default erased effective required state");

    app.setSuppressionRecursive(utils::ConfigSuppressionReason::Disabled, true);
    require(!requiredOpt->get_required(), "disabled owner still requires nested option");
    app.setSuppressionRecursive(utils::ConfigSuppressionReason::Disabled, false);
    require(requiredOpt->get_required(), "re-enabled owner left stale non-required CLI11 state");

    return 0;
}
