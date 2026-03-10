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
        , parent(parent)
        , final(final) {
        if (appWithPtr != nullptr) {
            if (parent != nullptr) {
                parent->subCommandApp->add_subcommand(appWithPtr);
            } else {
                subCommandAppOwner = std::move(appWithPtr);
            }

            subCommandApp->group(group);
            subCommandApp->configurable(false);
            subCommandApp->fallthrough();

            static const std::shared_ptr<CLI::HelpFormatter> helpFormatter = std::make_shared<CLI::HelpFormatter>();
            subCommandApp->formatter(helpFormatter);

            static const std::shared_ptr<CLI::Config> configFormatter = std::make_shared<CLI::ConfigFormatter>();
            subCommandApp->config_formatter(configFormatter);

            subCommandApp->option_defaults()->take_last()->group(subCommandApp->get_formatter()->get_label("Nonpersistent Options"));

            if (!final) {
                helpOpt = setConfigurable(subCommandApp
                                              ->set_help_flag(
                                                  "--help{exact},-h{exact}",
                                                  [subCommandApp = this->subCommandApp](std::size_t) {
                                                      helpTriggerApp = subCommandApp;
                                                  },
                                                  "Print help message and exit\n"
                                                  "* standard: display help for the last command processed\n"
                                                  "* exact: display help for the command directly preceding --help")
                                              ->take_first()
                                              ->check(CLI::IsMember({"standard", "exact", "expanded"}))
                                              ->trigger_on_parse(),
                                          false);
            } else {
                helpOpt = setConfigurable(subCommandApp
                                              ->set_help_flag(
                                                  "--help{exact},-h{exact}",
                                                  [subCommandApp = this->subCommandApp](std::size_t) {
                                                      helpTriggerApp = subCommandApp;
                                                  },
                                                  "Print help message and exit\n"
                                                  "* standard: display help for the last command processed\n"
                                                  "* exact: display help for the command directly preceding --help")
                                              ->take_first()
                                              ->check(CLI::IsMember({"standard", "exact"}))
                                              ->trigger_on_parse(),
                                          false);
            }

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
                                                 ->take_first()
                                                 ->check(CLI::IsMember({"standard", "active", "complete", "required"}))
                                                 ->trigger_on_parse(),
                                             false);
        }
    }

    SubCommand::~SubCommand() {
    }

    std::string SubCommand::getName() const {
        return subCommandApp->get_name();
    }

    std::string SubCommand::version() const {
        return subCommandApp->version();
    }

    void SubCommand::parse(int argc, char* argv[]) {
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
        if (parent != nullptr) {
            parent->subCommandApp->remove_subcommand(this->subCommandApp);
        }
    }

    CLI::Option* SubCommand::setConfig(const std::string& defaultConfigFile) const {
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

    CLI::Option* utils::SubCommand::setLogFile(const std::string& defaultLogFile) const {
        return addOption("-l,--log-file", "Log file", "logFile", defaultLogFile, !CLI::ExistingDirectory);
    }

    CLI::Option* SubCommand::setVersionFlag(const std::string& version) const {
        return subCommandApp->set_version_flag("-v,--version", version, "Framework version");
    }

    bool SubCommand::hasParent() const {
        return subCommandApp->get_parent() != nullptr;
    }

    SubCommand* SubCommand::getParent() const {
        return parent;
    }

    SubCommand* SubCommand::setRequireCallback(const std::function<void()>& callback) {
        subCommandApp->require_callback(callback);

        return this;
    }

    SubCommand* SubCommand::allowExtras(bool allow) {
        subCommandApp->allow_extras(allow);

        return this;
    }

    CLI::App* SubCommand::getCommandlineTriggerApp() {
        return commandlineTriggerApp;
    }

    CLI::App* SubCommand::getShowConfigTriggerApp() {
        return showConfigTriggerApp;
    }

    CLI::App* SubCommand::getHelpTriggerApp() {
        return helpTriggerApp;
    }

    SubCommand* SubCommand::required(bool required, bool force) {
        requiredCount += required ? 1 : -1;

        required = force ? required : requiredCount > 0;

        subCommandApp->required(required);

        SubCommand* parent = getParent();
        if (parent != nullptr) {
            if (required) {
                parent->needs(this);
            }

            parent->required(this, required);
        }

        return this;
    }

    SubCommand* SubCommand::required(SubCommand* subCommand, bool required) {
        if (subCommandApp->get_required() != required) {
            if (required) {
                needs(subCommand);

                for (const auto& sub : subCommand->subCommandApp->get_subcommands([](const CLI::App* sc) -> bool {
                         return sc->get_required();
                     })) {
                    subCommand->subCommandApp->needs(sub);
                }
            } else {
                needs(subCommand, false);

                for (const auto& sub : subCommand->subCommandApp->get_subcommands([](const CLI::App* sc) -> bool {
                         return sc->get_required();
                     })) {
                    subCommand->subCommandApp->remove_needs(sub);
                }
            }

            this->required(required, false);

            subCommand->subCommandApp->required(required);
            subCommand->subCommandApp->ignore_case(required);
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

            this->required(required, false);
        }

        return this;
    }

    SubCommand* SubCommand::needs(SubCommand* subCommand, bool needs) {
        if (needs) {
            subCommandApp->needs(subCommand->subCommandApp);
        } else {
            subCommandApp->remove_needs(subCommand->subCommandApp);
        }

        return this;
    }

    SubCommand* SubCommand::disabled(SubCommand* subCommand, bool disabled) {
        if (disabled) {
            if (subCommand->subCommandApp->get_ignore_case()) {
                needs(subCommand, false);
            }

            for (const auto& sub : subCommand->subCommandApp->get_subcommands({})) {
                if (sub->get_ignore_case()) {
                    subCommand->subCommandApp->remove_needs(sub);
                    sub->required(false);
                }
            }
        } else {
            if (subCommand->subCommandApp->get_ignore_case()) {
                needs(subCommand);
            }

            for (const auto& sub : subCommand->subCommandApp->get_subcommands({})) {
                if (sub->get_ignore_case()) {
                    subCommand->subCommandApp->needs(sub);
                    sub->required();
                }
            }
        }

        subCommand->subCommandApp->required(disabled ? false : subCommand->subCommandApp->get_ignore_case());

        return this;
    }

    SubCommand* SubCommand::finalCallback(const std::function<void()>& finalCallback) {
        subCommandApp->final_callback(finalCallback);

        return this;
    }

    std::string SubCommand::configToStr() const {
        return subCommandApp->config_to_str(true, true);
    }

    std::string SubCommand::help(const CLI::App* helpApp, const CLI::AppFormatMode& mode) const {
        return subCommandApp->help(helpApp, "", mode);
    }

    static std::shared_ptr<CLI::HelpFormatter> makeSectionFormatter() {
        const std::shared_ptr<CLI::HelpFormatter> sectionFormatter = std::make_shared<CLI::HelpFormatter>();

        sectionFormatter->label("SUBCOMMAND", "SECTION");
        sectionFormatter->label("SUBCOMMANDS", "SECTIONS");
        sectionFormatter->label("PERSISTENT", "");
        sectionFormatter->label("Persistent Options", "Options (persistent)");
        sectionFormatter->label("Nonpersistent Options", "Options (nonpersistent)");
        sectionFormatter->label("Usage", "\nUsage");
        sectionFormatter->label("bool:{true,false}", "{true,false}");
        sectionFormatter->label(":{standard,active,complete,required}", "{standard,active,complete,required}");
        sectionFormatter->label(":{standard,exact,expanded}", "{standard,exact,expanded}");
        sectionFormatter->column_width(7);

        return sectionFormatter;
    }

    std::shared_ptr<CLI::Formatter> SubCommand::sectionFormatter = makeSectionFormatter();

    CLI::Option* SubCommand::getOption(const std::string& name) const {
        return subCommandApp->get_option_no_throw(name);
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
                                               std::function<void(const std::string&)>& callback,
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
                                             const CLI::Validator& validator) {
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
                                             const CLI::Validator& validator) {
        return setDefaultValue(addFlagFunction(name, callback, description, typeName, validator), defaultValue);
    }

    CLI::Option* SubCommand::setConfigurable(CLI::Option* option, bool configurable) const {
        return option //
            ->configurable(configurable)
            ->group(subCommandApp->get_formatter()->get_label(configurable ? "Persistent Options" : "Nonpersistent Options"));
    }

    CLI::Option*
    SubCommand::initialize(CLI::Option* option, const std::string& typeName, const CLI::Validator& validator, bool configurable) const {
        return setConfigurable(option, configurable) //
            ->type_name(typeName)
            ->check(validator);
    }

    AppWithPtr::AppWithPtr(const std::string& description, const std::string& name, SubCommand* t, bool manage)
        : CLI::App(description, name)
        , ptr(t)
        , manage(manage) {
    }

    AppWithPtr::~AppWithPtr() {
        if (manage) {
            delete ptr;
        }
    }

    const SubCommand* AppWithPtr::getPtr() const {
        return ptr;
    }

    SubCommand* AppWithPtr::getPtr() {
        return ptr;
    }

    bool AppWithPtr::getManaged() const {
        return manage;
    }

    std::map<std::string, std::string> SubCommand::aliases;

    CLI::App* SubCommand::helpTriggerApp = nullptr;
    CLI::App* SubCommand::showConfigTriggerApp = nullptr;
    CLI::App* SubCommand::commandlineTriggerApp = nullptr;

} // namespace utils
