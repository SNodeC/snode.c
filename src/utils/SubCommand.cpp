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

            static const std::shared_ptr<CLI::HelpFormatter> helpFormatter = std::make_shared<CLI::HelpFormatter>();
            subCommandApp->formatter(helpFormatter);

            static const std::shared_ptr<CLI::Config> configFormatter = std::make_shared<CLI::ConfigFormatter>();
            subCommandApp->config_formatter(configFormatter);

            subCommandApp->option_defaults()->take_last()->group(subCommandApp->get_formatter()->get_label("Nonpersistent Options"));

            helpOpt = setConfigurable(subCommandApp
                                          ->set_help_flag(
                                              "--help{exact},-h{exact}",
                                              [subCommandApp = this->subCommandApp](std::size_t) {
                                                  helpTriggerApp = subCommandApp;
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
                                                        showConfigTriggerApp = subCommandApp;
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
                                                         commandlineTriggerApp = subCommandApp;
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
    }

    std::string SubCommand::getName() const {
        return name;
    }

    std::string SubCommand::version() const {
        return subCommandApp->version();
    }

    void SubCommand::parse(int argc, char* argv[]) const {
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
        for (const SubCommand* child : childSubCommands) {
            delete child;
        }
        childSubCommands.clear();

        if (parent != nullptr) {
            parent->subCommandApp->remove_subcommand(subCommandApp);
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

    SubCommand* SubCommand::allowExtras(bool allow) {
        subCommandApp->allow_extras(allow);

        return this;
    }

    SubCommand* SubCommand::forceUnrequired(bool unrequired) {
        const bool previouslyRequired = subCommandApp->get_required();
        const bool countedRequired = subCommandApp->get_ignore_case();

        requiredForced = unrequired;

        const bool effectiveRequired = requiredForced ? false : countedRequired;
        if (previouslyRequired != effectiveRequired) {
            subCommandApp->required(effectiveRequired);

            if (hasParent()) {
                SubCommand* parent = getParent();
                parent->needs(this, effectiveRequired);
                parent->required(effectiveRequired);
            }
        }

        return this;
    }

    SubCommand* SubCommand::required(bool required) {
        const bool previousEffectiveRequired = subCommandApp->get_required();

        requiredCount += required ? 1 : -1;
        const bool countedRequired = requiredCount > 0;
        const bool effectiveRequired = requiredForced ? false : countedRequired;

        subCommandApp->ignore_case(countedRequired);
        if (subCommandApp->get_required() != effectiveRequired) {
            subCommandApp->required(effectiveRequired);
        }

        if (hasParent() && previousEffectiveRequired != effectiveRequired) {
            SubCommand* parent = getParent();

            parent->needs(this, effectiveRequired);
            parent->required(effectiveRequired);
        }

        return this;
    }

    SubCommand* SubCommand::required(CLI::Option* option, bool required) {
        if (option->get_required() != required) {
            option->required(required);

            if (required) {
                subCommandApp->needs(option);
                option->default_str("");
            } else {
                subCommandApp->remove_needs(option);
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
            subCommandApp->needs(subCommand->subCommandApp);
        } else {
            subCommandApp->remove_needs(subCommand->subCommandApp);
        }

        return this;
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

    std::map<std::string, std::string> SubCommand::aliases;

    CLI::App* SubCommand::helpTriggerApp = nullptr;
    CLI::App* SubCommand::showConfigTriggerApp = nullptr;
    CLI::App* SubCommand::commandlineTriggerApp = nullptr;

} // namespace utils
