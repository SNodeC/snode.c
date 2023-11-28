/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "net/config/ConfigInstance.h"

#include "utils/ResetToDefault.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/Config.h"
#include "utils/Exceptions.h"

#include <cstdint>
#include <memory>
#include <stdexcept>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    const std::string ConfigInstance::nameAnonymous = "<anonymous>";

    ConfigInstance::ConfigInstance(const std::string& instanceName, const std::string& role)
        : instanceName(instanceName) {
        instanceSc = utils::Config::add_instance(instanceName, "Configuration for " + role + " instance '" + instanceName + "'");

        disableOpt = instanceSc
                         ->add_flag_function(
                             "--disabled",
                             [this]([[maybe_unused]] int64_t count) -> void {
                                 utils::Config::required(instanceSc, !disableOpt->as<bool>());
                             },
                             "Disable this instance")
                         ->trigger_on_parse()
                         ->take_last()
                         ->default_val("false")
                         ->type_name("bool")
                         ->check(CLI::IsMember({"true", "false"}))
                         ->group(instanceSc->get_formatter()->get_label("Persistent Options"));

        instanceSc->final_callback([this]() -> void {
            (utils::ResetToDefault(disableOpt))(disableOpt->as<std::string>());
        });
    }

    ConfigInstance::~ConfigInstance() {
        utils::Config::remove_instance(instanceSc);
    }

    const std::string& ConfigInstance::getInstanceName() const {
        return instanceName.empty() ? nameAnonymous : instanceName;
    }

    void ConfigInstance::setInstanceName(const std::string& instanceName) {
        this->instanceName = instanceName;
    }

    CLI::App* ConfigInstance::add_section(const std::string& name, const std::string& description) {
        CLI::App* sectionSc = instanceSc //
                                  ->add_subcommand(name, description)
                                  ->fallthrough()
                                  ->configurable(false)
                                  ->allow_extras(instanceSc->get_allow_extras())
                                  ->group("Sections")
                                  ->disabled(this->instanceName.empty());

        sectionSc //
            ->option_defaults()
            ->configurable(!this->instanceName.empty());

        sectionSc
            ->add_flag_callback( //
                "-s,--show-config",
                [sectionSc]() {
                    throw CLI::CallForShowConfig(sectionSc);
                },
                "Show current configuration and exit") //
            ->configurable(false)
            ->disable_flag_override();

        sectionSc
            ->add_flag_callback(
                "--command-line",
                [sectionSc]() {
                    throw CLI::CallForCommandline(sectionSc, CLI::CallForCommandline::Mode::REQUIRED);
                },
                "Print a template command line showing required options only")
            ->configurable(false)
            ->disable_flag_override();

        sectionSc
            ->add_flag_callback(
                "--command-line-full",
                [sectionSc]() {
                    throw CLI::CallForCommandline(sectionSc, CLI::CallForCommandline::Mode::ALL);
                },
                "Print a template command line showing all possible options")
            ->configurable(false)
            ->disable_flag_override();

        sectionSc
            ->add_flag_callback(
                "--command-line-configured",
                [sectionSc]() {
                    throw CLI::CallForCommandline(sectionSc, CLI::CallForCommandline::Mode::CONFIGURED);
                },
                "Print a template command line showing all required and configured options") //
            ->configurable(false)
            ->disable_flag_override();

        sectionSc //
            ->add_flag_callback(
                "-h,--help",
                []() {
                    throw CLI::CallForHelp();
                },
                "Print this help message and exit")
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();

        sectionSc //
            ->add_flag_callback(
                "-a,--help-all",
                []() {
                    throw CLI::CallForAllHelp();
                },
                "Print this help message and exit")
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();

        return sectionSc;
    }

    void ConfigInstance::required(CLI::App* section, bool req) {
        if (req != section->get_required()) {
            if (req) {
                ++requiredCount;
                instanceSc->needs(section);
            } else {
                --requiredCount;
                instanceSc->remove_needs(section);
            }

            section->required(req);

            if (!disableOpt->as<bool>()) {
                utils::Config::required(instanceSc, requiredCount > 0);
            }
        }
    }

    bool ConfigInstance::getRequired() const {
        return requiredCount > 0;
    }

    bool ConfigInstance::getDisabled() const {
        return disableOpt //
            ->as<bool>();
    }

    void ConfigInstance::setDisabled(bool disabled) {
        disableOpt //
            ->default_val(disabled ? "true" : "false")
            ->clear();

        utils::Config::required(instanceSc, !disabled);
    }

} // namespace net::config
