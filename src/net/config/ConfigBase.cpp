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

#include "net/config/ConfigBase.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"
#include "utils/Config.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    ConfigBase::ConfigBase(const std::string& name)
        : name(name) {
        if (!name.empty()) {
            baseSc = utils::Config::add_subcommand(name, "Configuration for instance '" + name + "'");
            baseSc->fallthrough();
            baseSc->group("Instances");
        }
    }

    ConfigBase::~ConfigBase() {
        utils::Config::remove_subcommand(baseSc);
    }

    const std::string& ConfigBase::getName() const {
        return name;
    }

    CLI::App* ConfigBase::add_subcommand(const std::string& name, const std::string& description) {
        CLI::App* subCommand = baseSc->add_subcommand(name, description);
        baseSc->require_subcommand(baseSc->get_require_subcommand_min(), baseSc->get_require_subcommand_max() + 1);
        subCommand->group("Sections");

        subCommand
            ->add_flag_callback(
                "-h,--help",
                []() {
                    throw CLI::CallForHelp();
                },
                "Print this help message and exit")
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();
        //            ->group("General Options");

        subCommand
            ->add_flag_callback(
                "--help-all",
                []() {
                    throw CLI::CallForAllHelp();
                },
                "Expand all help")
            ->configurable(false)
            ->disable_flag_override()
            ->trigger_on_parse();
        //            ->group("General Options");

        return subCommand;
    }

    CLI::Option* ConfigBase::add_option(const std::string& name, int& variable, const std::string& description) {
        return baseSc->add_option(name, variable, description);
    }

    CLI::Option* ConfigBase::add_option(const std::string& name, std::string& variable, const std::string& description) {
        return baseSc->add_option(name, variable, description);
    }

    CLI::Option* ConfigBase::add_flag(const std::string& name, const std::string& description) {
        return baseSc->add_flag(name, description);
    }

    void ConfigBase::required(bool reqired) {
        if (baseSc != nullptr) {
            baseSc->required(reqired);
        }
    }

    void ConfigBase::parse() {
        utils::Config::parse();
    }

} // namespace net::config
