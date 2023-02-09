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

#include "utils/CLI11.hpp"
#include "utils/Config.h"

#include <stdexcept>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    ConfigInstance::ConfigInstance(const std::string& name)
        : name(name) {
        instanceSc = utils::Config::add_instance(name, "Configuration for instance '" + name + "'");

        disabledOpt = instanceSc->add_flag_function("--disable", utils::ResetToDefault(disabledOpt), "Disable this instance")
                          ->configurable(true)
                          ->take_last()
                          ->default_val("false")
                          ->type_name("bool")
                          ->check(CLI::TypeValidator<bool>() & !CLI::Number); // cppcheck-suppress clarifyCondition
    }

    ConfigInstance::~ConfigInstance() {
        utils::Config::remove_instance(instanceSc);
    }

    const std::string& ConfigInstance::getInstanceName() const {
        return name;
    }

    CLI::App* ConfigInstance::add_section(const std::string& name, const std::string& description) {
        CLI::App* sectionSc = instanceSc //
                                  ->add_subcommand(name, description)
                                  ->configurable(false)
                                  ->group(this->name.empty() ? "" : "Sections")
                                  ->disabled(this->name.empty())
                                  ->silent(this->name.empty());

        sectionSc //
            ->option_defaults()
            ->configurable(!this->name.empty());

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
                "--help-all",
                []() {
                    throw CLI::CallForAllHelp();
                },
                "Expand all help")
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();

        return sectionSc;
    }

    void ConfigInstance::required(bool reqired) {
        if (instanceSc != nullptr) {
            instanceSc //
                ->required(reqired);
        }
    }

    bool ConfigInstance::getDisabled() const {
        return disabledOpt->as<bool>();
    }

    void ConfigInstance::setDisabled(bool disabled) {
        disabledOpt //
            ->default_val(disabled ? "true" : "false")
            ->take_all()
            ->clear();
    }

} // namespace net::config
