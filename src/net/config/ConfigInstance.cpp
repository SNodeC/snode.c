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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"
#include "utils/Config.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    ConfigInstance::ConfigInstance(const std::string& name)
        : name(name) {
        instanceSc = utils::Config::add_instance(name, "Configuration for instance '" + name + "'") //
                         ->fallthrough()                                                            //
                         ->group("Instances");

        instanceSc->disabled(getInstanceName().empty());
    }

    ConfigInstance::~ConfigInstance() {
        utils::Config::remove_instance(instanceSc);
    }

    const std::string& ConfigInstance::getInstanceName() const {
        return name;
    }

    CLI::App* ConfigInstance::add_section(const std::string& name, const std::string& description) {
        CLI::App* subCommand = instanceSc                              //
                                   ->add_subcommand(name, description) //
                                   ->group("Sections");

        subCommand //
            ->add_flag_callback(
                "-h,--help",
                []() {
                    throw CLI::CallForHelp();
                },
                "Print this help message and exit") //
            ->configurable(false)                   //
            ->disable_flag_override()               //
            ->trigger_on_parse();

        subCommand //
            ->add_flag_callback(
                "--help-all",
                []() {
                    throw CLI::CallForAllHelp();
                },
                "Expand all help")    //
            ->configurable(false)     //
            ->disable_flag_override() //
            ->trigger_on_parse();

        instanceSc->require_subcommand(instanceSc->get_require_subcommand_min(), instanceSc->get_require_subcommand_max() + 1);

        return subCommand;
    }

    void ConfigInstance::required(bool reqired) {
        if (instanceSc != nullptr) {
            instanceSc //
                ->required(reqired);
        }
    }

} // namespace net::config
