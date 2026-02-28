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

#include "ConfigApp.h"
#include "utils/Formatter.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <memory>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace utils {

    SubCommand::SubCommand(CLI::App* subCommandSc)
        : subCommandSc(subCommandSc) {
        setConfigurable(addOption( //
                            "-i,--instance-alias",
                            "Make an instance also known as an alias in configuration files",
                            "instance=instance_alias [instance=instance_alias [...]]",
                            CLI::TypeValidator<std::string>())
                            ->each([&aliases = this->aliases](const std::string& item) {
                                const auto it = item.find('=');
                                if (it != std::string::npos) {
                                    aliases[item.substr(0, it)] = item.substr(it + 1);
                                } else {
                                    throw CLI::ConversionError("Can not convert '" + item + "' to a 'instance=instance_alias' pair");
                                }
                            }),
                        false);
    }

    SubCommand* SubCommand::required(bool required, bool force) {
        requiredCount += !force ? (required ? 1 : -1) : 0;

        subCommandSc->required(force ? required : requiredCount > 0);

        CLI::App* parentSc = subCommandSc->get_parent();

        if (parentSc != nullptr) {
            parentSc->required(force ? required : requiredCount > 0);
        }

        return this;
    }

    SubCommand* SubCommand::setRequireCallback(const std::function<void()>& callback) {
        subCommandSc->require_callback(callback);

        return this;
    }

    CLI::App* SubCommand::getParent() const {
        return subCommandSc->get_parent();
    }

    SubCommand* SubCommand::addStandardFlags() {
        setConfigurable(addFlagFunction(
                            "-s,--show-config",
                            [subCommandSc = this->subCommandSc, &showConfigTriggerApp = this->showConfigTriggerApp](std::size_t) {
                                if (showConfigTriggerApp == nullptr) {
                                    showConfigTriggerApp = subCommandSc;
                                }
                            },
                            "Show current configuration and exit",
                            "",
                            CLI::TypeValidator<bool>())
                            ->take_first()
                            ->disable_flag_override()
                            ->configurable(false)
                            ->trigger_on_parse(),
                        false);

        setConfigurable(addFlagFunction(
                            "--command-line{standard}",
                            [subCommandSc = this->subCommandSc,
                             &commandlineTriggerApp = this->commandlineTriggerApp]([[maybe_unused]] std::int64_t count) {
                                if (commandlineTriggerApp == nullptr) {
                                    commandlineTriggerApp = subCommandSc;
                                }
                            },
                            "Print command-line\n"
                            "* standard (default): Show all non-default and required options\n"
                            "* active: Show all active options\n"
                            "* complete: Show the complete option set with default values\n"
                            "* required: Show only required options",
                            "",
                            CLI::TypeValidator<bool>())
                            ->take_first()
                            ->disable_flag_override()
                            ->trigger_on_parse(),
                        false);

        return this;
    }

    SubCommand* SubCommand::addSimpleHelp() {
        setConfigurable(subCommandSc
                            ->set_help_flag(
                                "--help{exact},-h{exact}",
                                [subCommandSc = this->subCommandSc, &helpTriggerApp = this->helpTriggerApp](std::size_t) {
                                    if (helpTriggerApp == nullptr) {
                                        helpTriggerApp = subCommandSc;
                                    }
                                },
                                "Print help message and exit\n"
                                "* standard: display help for the last command processed\n"
                                "* exact: display help for the command directly preceding --help")
                            ->take_first()
                            ->check(CLI::IsMember({"standard", "exact"}))
                            ->trigger_on_parse(),
                        false);

        return this;
    }

    SubCommand* SubCommand::addHelp() {
        subCommandSc
            ->set_help_flag(
                "-h{exact},--help{exact}",
                [subCommandSc = this->subCommandSc, &helpTriggerApp = this->helpTriggerApp](std::size_t) {
                    if (helpTriggerApp == nullptr) {
                        helpTriggerApp = subCommandSc;
                    }
                },
                "Print help message and exit\n"
                "* standard: display help for the last command processed\n"
                "* exact: display help for the command directly preceding --help\n"
                "* expanded: print help including all descendant command options")
            ->take_first()
            ->check(CLI::IsMember({"standard", "exact", "expanded"}))
            ->trigger_on_parse();

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

    SubCommand* SubCommand::required(SubCommand* instance, bool required) {
        if (required != instance->subCommandSc->get_required()) {
            if (required) {
                subCommandSc->needs(instance->subCommandSc);

                for (const auto& sub : instance->subCommandSc->get_subcommands([](const CLI::App* sc) -> bool {
                         return sc->get_required();
                     })) {
                    instance->subCommandSc->needs(sub);
                }
            } else {
                subCommandSc->remove_needs(instance->subCommandSc);

                for (const auto& sub : instance->subCommandSc->get_subcommands([](const CLI::App* sc) -> bool {
                         return sc->get_required();
                     })) {
                    instance->subCommandSc->remove_needs(sub);
                }
            }

            instance->subCommandSc->required(required);
            instance->subCommandSc->ignore_case(required);

            this->required(required, false);
        }
        return this;
    }

    SubCommand* SubCommand::required(CLI::Option* option, bool required) {
        if (required != option->get_required()) {
            if (required) {
                subCommandSc->needs(option);
                option->default_str("");
            } else {
                subCommandSc->remove_needs(option);
            }

            option->required(required);

            this->required(required, false);
        }

        return this;
    }

    SubCommand* SubCommand::disabled(SubCommand* instance, bool disabled) {
        if (disabled) {
            if (instance->subCommandSc->get_ignore_case()) {
                subCommandSc->remove_needs(instance->subCommandSc);
            }

            for (const auto& sub : instance->subCommandSc->get_subcommands({})) {
                if (sub->get_ignore_case()) {
                    instance->subCommandSc->remove_needs(sub);
                    sub->required(false); // ### must be stored in ConfigInstance
                }
            }
        } else {
            if (instance->subCommandSc->get_ignore_case()) {
                subCommandSc->needs(instance->subCommandSc);
            }

            for (const auto& sub : instance->subCommandSc->get_subcommands({})) {
                if (sub->get_ignore_case()) { // ### must be recalled from ConfigInstance
                    instance->subCommandSc->needs(sub);
                    sub->required();
                }
            }
        }

        instance->subCommandSc->required(disabled ? false : instance->subCommandSc->get_ignore_case());

        return this;
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

    utils::SubCommand*
    SubCommand::newInstance(std::shared_ptr<utils::AppWithPtr<SubCommand>> appWithPtr, const std::string& group, bool final) {
        CLI::App* instanceSc = subCommandSc->add_subcommand(appWithPtr)
                                   ->group(group)
                                   ->ignore_case(false)
                                   ->fallthrough()
                                   ->formatter(sectionFormatter)
                                   ->configurable(false)
                                   ->config_formatter(subCommandSc->get_config_formatter())
                                   ->allow_extras()
                                   ->disabled(appWithPtr->get_name().empty());

        instanceSc //
            ->option_defaults()
            ->configurable(!instanceSc->get_disabled());

        if (!instanceSc->get_disabled()) {
            if (aliases.contains(instanceSc->get_name())) {
                instanceSc //
                    ->alias(aliases[instanceSc->get_name()]);
            }
        }

        SubCommand* newSubCommandSc = appWithPtr->getPtr();

        newSubCommandSc->addStandardFlags();

        if (!final) {
            newSubCommandSc->addHelp();
        } else {
            newSubCommandSc->addSimpleHelp();
        }

        return newSubCommandSc;
    }

    SubCommand* SubCommand::removeInstance(utils::SubCommand* instance) {
        required(instance, false);

        return this;
    }

    CLI::Option* SubCommand::getOption(const std::string& name) const {
        return subCommandSc->get_option(name);
    }

    CLI::Option* SubCommand::addOption(const std::string& name,
                                       const std::string& description,
                                       const std::string& typeName,
                                       const CLI::Validator& validator) {
        return initialize(subCommandSc //
                              ->add_option(name, description),
                          typeName,
                          validator,
                          !subCommandSc->get_disabled());
    }

    CLI::Option* SubCommand::addOptionFunction(const std::string& name,
                                               std::function<void(const std::string&)>& callback,
                                               const std::string& description,
                                               const std::string& typeName,
                                               const CLI::Validator& validator) {
        return initialize(subCommandSc //
                              ->add_option_function(name, callback, description),
                          typeName,
                          validator,
                          !subCommandSc->get_disabled());
    }

    CLI::Option* SubCommand::addFlag(const std::string& name,
                                     const std::string& description,
                                     const std::string& typeName,
                                     const CLI::Validator& validator) {
        return initialize(subCommandSc //
                              ->add_flag(name, description),
                          typeName,
                          validator,
                          !subCommandSc->get_disabled());
    }

    CLI::Option* SubCommand::addFlagFunction(const std::string& name,
                                             const std::function<void(std::int64_t)>& callback,
                                             const std::string& description,
                                             const std::string& typeName,
                                             const CLI::Validator& validator) {
        return initialize(subCommandSc //
                              ->add_flag_function(name, callback, description),
                          typeName,
                          validator,
                          !subCommandSc->get_disabled());
    }

    CLI::Option* SubCommand::setConfigurable(CLI::Option* option, bool configurable) const {
        option //
            ->configurable(configurable)
            ->group(subCommandSc->get_formatter()->get_label(configurable ? "Persistent Options" : "Nonpersistent Options"));

        return option;
    }

    CLI::Option*
    SubCommand::initialize(CLI::Option* option, const std::string& typeName, const CLI::Validator& validator, bool configurable) const {
        return setConfigurable(option, configurable) //
            ->type_name(typeName)
            ->check(validator);

        return option;
    }

} // namespace utils
