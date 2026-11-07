/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "SubCommand.h"

#include "utils/Formatter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    SubCommand::SubCommand(SubCommand* parent, std::shared_ptr<utils::AppWithPtr> appWithPtr, const std::string& group, bool final)
        : subCommandApp(appWithPtr.get())
        , name(subCommandApp != nullptr ? subCommandApp->get_name() : "<NOAPP>")
        , parent(parent)
        , final(final) {
        if (appWithPtr != nullptr) {
            if (parent != nullptr) {
                parent->addSubCommandApp(appWithPtr);
            } else {
                selfAnchoredSubCommandApp = std::move(appWithPtr);
            }

            subCommandApp->group(group);
            subCommandApp->configurable(false);
            subCommandApp->fallthrough();
            subCommandApp->show_fallthrough_parent_options(false);

            static const std::shared_ptr<CLI::HelpFormatter> helpFormatter = std::make_shared<CLI::HelpFormatter>();
            subCommandApp->formatter(helpFormatter);

            static const std::shared_ptr<CLI::Config> configFormatter = std::make_shared<CLI::ConfigFormatter>();
            subCommandApp->config_formatter(configFormatter);

            subCommandApp->option_defaults()->take_last()->group(subCommandApp->get_formatter()->get_label("Nonpersistent Options"));
            subCommandApp->option_defaults()->callback_priority(CLI::CallbackPriority::PreRequirementsCheckPreHelp);

            helpOpt = setConfigurable(subCommandApp
                                          ->set_help_flag(
                                              "--help{exact},-h{exact}",
                                              [subCommandApp = this->subCommandApp](std::size_t) {
                                                  if (helpTriggerApp == nullptr) {
                                                      helpTriggerApp = subCommandApp;
                                                  }
                                              },
                                              "Print help message and exit\n"
                                              "* standard: display help for the last command processed\n"
                                              "* exact: display help for the command directly preceding --help")
                                          ->type_name("MODE")
                                          ->take_first()
                                          ->check(CLI::IsMember({"standard", "exact"}))
                                          ->trigger_on_parse(),
                                      false);

            showConfigOpt = setConfigurable(subCommandApp
                                                ->add_flag_function(
                                                    "-s,--show-config",
                                                    [subCommandApp = this->subCommandApp](std::size_t) {
                                                        if (showConfigTriggerApp == nullptr) {
                                                            showConfigTriggerApp = subCommandApp;
                                                        }
                                                    },
                                                    "Show current configuration and exit")
                                                ->take_first()
                                                ->disable_flag_override()
                                                ->configurable(false)
                                                ->trigger_on_parse(),
                                            false);

            commandlineOpt = setConfigurable(subCommandApp
                                                 ->add_flag_function(
                                                     "--command-line{standard}",
                                                     [subCommandApp = this->subCommandApp]([[maybe_unused]] std::int64_t count) {
                                                         if (commandlineTriggerApp == nullptr) {
                                                             commandlineTriggerApp = subCommandApp;
                                                         }
                                                     },
                                                     "Print command-line\n"
                                                     "* standard (default): Show all non-default and required options\n"
                                                     "* active: Show all active options\n"
                                                     "* complete: Show the complete option set with default values\n"
                                                     "* required: Show only required options")
                                                 ->type_name("MODE")
                                                 ->take_first()
                                                 ->check(CLI::IsMember({"standard", "active", "complete", "required"}))
                                                 ->trigger_on_parse(),
                                             false);
        }
    }

    SubCommand::~SubCommand() {
        removeSubCommand();
    }

    std::string SubCommand::getName() const {
        return name;
    }

    std::string SubCommand::version() const {
        return subCommandApp->version();
    }

    void SubCommand::parse(int argc, char* argv[]) const {
        updatePromotedDefaults(true);
        subCommandApp->parse(argc, argv);
    }

    SubCommand* SubCommand::description(const std::string& description) {
        subCommandApp->description(description);

        return this;
    }

    SubCommand* SubCommand::footer(const std::string& footer) {
        subCommandApp->footer(footer);

        return this;
    }

    void SubCommand::removeSubCommand() {
        while (!childSubCommands.empty()) {
            SubCommand* child = *childSubCommands.begin();
            childSubCommands.erase(child);

            delete child;
        }

        if (parent != nullptr) {
            parent->childSubCommands.erase(this);

            if (parent->subCommandApp != nullptr && subCommandApp != nullptr) {
                parent->neededSubCommands.erase(this);
                parent->subCommandApp->remove_needs(subCommandApp);
                parent->subCommandApp->remove_subcommand(subCommandApp);
            }

            parent = nullptr;
        }
    }

    std::string SubCommand::configToStr() const {
        return subCommandApp->config_to_str(true, true);
    }

    std::string SubCommand::help(const CLI::App* helpApp, const CLI::AppFormatMode& mode) const {
        return subCommandApp->help(helpApp, "", mode);
    }

    bool SubCommand::hasParent() const {
        return subCommandApp->get_parent() != nullptr;
    }

    SubCommand* SubCommand::getParent() const {
        return parent;
    }

    const AppWithPtr* SubCommand::getAppWithPtr() const {
        return subCommandApp;
    }

    AppWithPtr* SubCommand::getAppWithPtr() {
        return subCommandApp;
    }

    SubCommand* SubCommand::allowExtras(bool allow) {
        subCommandApp->allow_extras(allow);

        return this;
    }

    SubCommand* SubCommand::forceUnrequired(bool unrequired) {
        requiredForced = unrequired;

        return suppressRequirements(ConfigSuppressionReason::ForceUnrequired, requiredForced);
    }

    SubCommand* SubCommand::suppressRequirements(ConfigSuppressionReason reason, bool suppressed) {
        const bool previousEffectiveRequired = requirementsActive();

        suppressRequirementsRecursive(reason, suppressed);

        applyEffectiveRequiredContribution(previousEffectiveRequired, requirementsActive());

        return this;
    }

    SubCommand* SubCommand::required(bool required) {
        const bool previousCanonicalRequired = requiredCount > 0;
        const bool previousEffectiveRequired = requirementsActive();

        if (required) {
            requiredCount++;
            effectiveRequiredCount++;
        } else {
            requiredCount = requiredCount > 0 ? requiredCount - 1 : 0;
            effectiveRequiredCount = effectiveRequiredCount > 0 ? effectiveRequiredCount - 1 : 0;
        }
        const bool countedRequired = requiredCount > 0;
        const unsigned forced = 1U << static_cast<unsigned>(ConfigSuppressionReason::ForceUnrequired);
        suppressions = requiredForced ? suppressions | forced : suppressions & ~forced;
        applyRequirements();
        const bool effectiveRequired = requirementsActive();

        if (hasParent() && previousCanonicalRequired != countedRequired) {
            SubCommand* parent = getParent();

            parent->needs(this, countedRequired);
            parent->applyCanonicalRequiredContribution(previousCanonicalRequired, countedRequired);
        }

        applyEffectiveRequiredContribution(previousEffectiveRequired, effectiveRequired);

        return this;
    }

    SubCommand* SubCommand::required(CLI::Option* option, bool required) {
        const auto state = options.find(option);
        const bool declaredRequired = state != options.end() && state->second.required;
        if (declaredRequired != required) {
            options[option].required = required;
            applyRequirements();

            if (required) {
                option->default_str("");
            }

            this->required(required);
        }

        return this;
    }

    bool SubCommand::getRequired() const {
        return subCommandApp->get_required();
    }

    SubCommand* SubCommand::needs(SubCommand* subCommand, bool needs) {
        if (needs) {
            neededSubCommands.try_emplace(subCommand, false);
        } else {
            const auto needed = neededSubCommands.find(subCommand);
            if (needed != neededSubCommands.end()) {
                if (needed->second) {
                    subCommandApp->remove_needs(subCommand->subCommandApp);
                }
                neededSubCommands.erase(needed);
            }
        }
        applyRequirements();

        return this;
    }

    void SubCommand::applyCanonicalRequiredContribution(bool previousCanonicalRequired, bool canonicalRequired) {
        if (previousCanonicalRequired != canonicalRequired) {
            const bool previousParentCanonicalRequired = requiredCount > 0;

            if (canonicalRequired) {
                requiredCount++;
            } else {
                requiredCount = requiredCount > 0 ? requiredCount - 1 : 0;
            }

            const bool parentCanonicalRequired = requiredCount > 0;
            applyRequirements();

            if (hasParent() && previousParentCanonicalRequired != parentCanonicalRequired) {
                SubCommand* parent = getParent();

                parent->needs(this, parentCanonicalRequired);
                parent->applyCanonicalRequiredContribution(previousParentCanonicalRequired, parentCanonicalRequired);
            }
        }
    }

    void SubCommand::applyEffectiveRequiredContribution(bool previousEffectiveRequired, bool effectiveRequired) {
        if (hasParent() && previousEffectiveRequired != effectiveRequired) {
            SubCommand* parent = getParent();
            const bool previousParentEffectiveRequired = parent->requirementsActive();

            if (effectiveRequired) {
                parent->effectiveRequiredCount++;
            } else {
                parent->effectiveRequiredCount = parent->effectiveRequiredCount > 0 ? parent->effectiveRequiredCount - 1 : 0;
            }
            parent->applyRequirements();

            parent->applyEffectiveRequiredContribution(previousParentEffectiveRequired, parent->requirementsActive());
        }
    }

    SubCommand* SubCommand::setRequireCallback(const std::function<void()>& callback) {
        subCommandApp->require_callback(callback);

        return this;
    }

    SubCommand* SubCommand::finalCallback(const std::function<void()>& finalCallback) {
        subCommandApp->final_callback(finalCallback);

        return this;
    }

    void SubCommand::addSubCommandApp(std::shared_ptr<AppWithPtr> subCommand) {
        if (subCommandApp->get_subcommands({}).empty()) {
            helpOpt->description("Print help message and exit\n"
                                 "* standard: display help for the last command processed\n"
                                 "* exact: display help for the command directly preceding --help\n"
                                 "* expanded: display help including all descendant sections");
            if (auto* existing = helpOpt->get_validator(); existing != nullptr) {
                *existing = std::move(CLI::IsMember({"standard", "exact", "expanded"})); // replace, do not append
            }
        }

        subCommandApp->add_subcommand(subCommand);
    }

    CLI::Option* SubCommand::addConfigFlag(const std::string& defaultConfigFile) const {
        return subCommandApp
            ->set_config( //
                "-c,--config-file",
                defaultConfigFile,
                "Read a config file",
                false) //
            ->take_all()
            ->type_name("configfile")
            ->check(!CLI::ExistingDirectory);
    }

    CLI::Option* utils::SubCommand::addLogFileFlag(const std::string& defaultLogFile) const {
        return addOption("-l,--log-file", "Log file", "logFile", defaultLogFile, !CLI::ExistingDirectory);
    }

    CLI::Option* SubCommand::addVersionFlag(const std::string& version) const {
        return subCommandApp->set_version_flag("-v,--version", version, "Framework version");
    }

    CLI::Option* SubCommand::addOption(const std::string& name,
                                       const std::string& description,
                                       const std::string& typeName,
                                       const CLI::Validator& validator) const {
        return initialize(subCommandApp //
                              ->add_option(name, description),
                          typeName,
                          validator,
                          !subCommandApp->get_disabled());
    }

    CLI::Option* SubCommand::addOptionFunction(const std::string& name,
                                               const std::function<void(const std::string&)>& callback,
                                               const std::string& description,
                                               const std::string& typeName,
                                               const CLI::Validator& validator) const {
        return initialize(subCommandApp //
                              ->add_option_function(name, callback, description),
                          typeName,
                          validator,
                          !subCommandApp->get_disabled());
    }

    CLI::Option* SubCommand::addFlag(const std::string& name,
                                     const std::string& description,
                                     const std::string& typeName,
                                     const CLI::Validator& validator) const {
        return initialize(subCommandApp //
                              ->add_flag(name, description),
                          typeName,
                          validator,
                          !subCommandApp->get_disabled());
    }

    CLI::Option* SubCommand::addFlagFunction(const std::string& name,
                                             const std::function<void()>& callback,
                                             const std::string& description,
                                             const std::string& typeName,
                                             const CLI::Validator& validator) const {
        CLI::Option* opt = subCommandApp //
                               ->add_flag_function(
                                   name,
                                   [callback](std::int64_t) {
                                       callback();
                                   },
                                   description)
                               ->type_name(typeName)
                               ->check(validator)
                               ->take_last();
        if (opt->get_configurable()) {
            opt->group(subCommandApp->get_formatter()->get_label("Persistent Options"));
        }

        return opt;
    }

    CLI::Option* SubCommand::addFlagFunction(const std::string& name,
                                             const std::function<void()>& callback,
                                             const std::string& description,
                                             const std::string& typeName,
                                             const std::string& defaultValue,
                                             const CLI::Validator& validator) const {
        return setDefaultValue(addFlagFunction(name, callback, description, typeName, validator), defaultValue);
    }

    CLI::Option* SubCommand::getOption(const std::string& name) const {
        return subCommandApp->get_option_no_throw(name);
    }

    CLI::Option* SubCommand::setConfigurable(CLI::Option* option, bool configurable) const {
        return option //
            ->configurable(configurable)
            ->group(subCommandApp->get_formatter()->get_label(configurable ? "Persistent Options" : "Nonpersistent Options"));
    }

    CLI::App* SubCommand::getHelpTriggerApp() {
        return helpTriggerApp;
    }

    CLI::App* SubCommand::getShowConfigTriggerApp() {
        return showConfigTriggerApp;
    }

    CLI::App* SubCommand::getCommandlineTriggerApp() {
        return commandlineTriggerApp;
    }

    CLI::Option*
    SubCommand::initialize(CLI::Option* option, const std::string& typeName, const CLI::Validator& validator, bool configurable) const {
        return setConfigurable(option, configurable) //
            ->type_name(typeName)
            ->check(validator);
    }

    AppWithPtr::AppWithPtr(const std::string& description, const std::string& name, SubCommand* t)
        : CLI::App(description, name)
        , ptr(t) {
    }

    AppWithPtr::~AppWithPtr() {
    }

    const SubCommand* AppWithPtr::getPtr() const {
        return ptr;
    }

    SubCommand* AppWithPtr::getPtr() {
        return ptr;
    }

    CLI::Option*
    AppWithPtr::set_help_flag(std::string flag_name, std::function<void(std::size_t)> help_callback, const std::string& help_description) {
        // take flag_description by const reference otherwise add_flag tries to assign to help_description
        if (help_ptr_ != nullptr) {
            remove_option(help_ptr_);
            help_ptr_ = nullptr;
        }

        // Empty name will simply remove the help flag
        if (!flag_name.empty()) {
            help_ptr_ = add_flag(flag_name, help_callback, help_description);
            help_ptr_->configurable(false);
            help_ptr_->callback_priority(CLI::CallbackPriority::PreRequirementsCheck);
        }

        return help_ptr_;
    }

    bool SubCommand::requirementsActive() const {
        return requiredCount > 0 && effectiveRequiredCount > 0 && suppressions == 0;
    }

    void SubCommand::applyRequirements() {
        subCommandApp->required(requirementsActive());
        for (auto& [option, state] : options) {
            option->required(state.required && state.suppressions == 0);
            if (state.needed) {
                subCommandApp->remove_needs(option);
            }
            state.needed = state.required && state.suppressions == 0 && suppressions == 0;
            if (state.needed) {
                subCommandApp->needs(option);
            }
        }
        for (auto& [command, needed] : neededSubCommands) {
            if (needed) {
                subCommandApp->remove_needs(command->subCommandApp);
            }
            needed = suppressions == 0 && command->requirementsActive();
            if (needed) {
                subCommandApp->needs(command->subCommandApp);
            }
        }
    }

    void SubCommand::suppressRequirementsRecursive(ConfigSuppressionReason reason, bool suppressed) {
        const unsigned bit = 1U << static_cast<unsigned>(reason);
        suppressions = suppressed ? suppressions | bit : suppressions & ~bit;
        for (CLI::Option* option : subCommandApp->get_options({})) {
            auto& state = options[option];
            state.suppressions = suppressed ? state.suppressions | bit : state.suppressions & ~bit;
        }
        applyRequirements();
        for (CLI::App* child : subCommandApp->get_subcommands({})) {
            if (auto* app = dynamic_cast<AppWithPtr*>(child); app != nullptr && app->getPtr() != nullptr) {
                app->getPtr()->suppressRequirementsRecursive(reason, suppressed);
            }
        }
        applyRequirements();
    }

    void SubCommand::promoteDefault(CLI::Option* option) {
        if (option->empty()) {
            option->add_result(option->get_default_str());
            option->run_callback();
        }
        auto [state, inserted] = options.try_emplace(option);
        if (inserted) {
            state->second.required = option->get_required();
        }
        state->second.promoted = true;
    }

    void SubCommand::updatePromotedDefaults(bool clear) const {
        for (const auto& [option, state] : options) {
            if (state.promoted) {
                if (clear) {
                    option->clear();
                } else if (option->empty()) {
                    option->add_result(option->get_default_str());
                    option->run_callback();
                }
            }
        }
        for (CLI::App* child : subCommandApp->get_subcommands({})) {
            if (auto* app = dynamic_cast<AppWithPtr*>(child); app != nullptr && app->getPtr() != nullptr) {
                app->getPtr()->updatePromotedDefaults(clear);
            }
        }
    }

    void AppWithPtr::pre_callback() {
        if (ptr != nullptr) {
            ptr->updatePromotedDefaults(false);
        }
    }

    bool SubCommand::isDefaultPromoted(const CLI::Option* option) const {
        const auto state = options.find(option);

        return state != options.end() && state->second.promoted;
    }

    void OptionView::promoteDefault() {
        config.promoteDefault(option);
    }

    OptionView SubCommand::option(const std::string& name) {
        return {*this, subCommandApp->get_option(name)};
    }

    std::map<std::string, std::string> SubCommand::aliases;

    CLI::App* SubCommand::helpTriggerApp = nullptr;
    CLI::App* SubCommand::showConfigTriggerApp = nullptr;
    CLI::App* SubCommand::commandlineTriggerApp = nullptr;

} // namespace utils
