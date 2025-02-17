/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
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

#include "utils/Config.h"

#include <functional>
#include <memory>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#ifdef __has_warning
#if __has_warning("-Wweak-vtables")
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#endif
#endif
#include "utils/CLI11.hpp"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    const std::string ConfigInstance::nameAnonymous = "<anonymous>";

    ConfigInstance::ConfigInstance(const std::string& instanceName, Role role)
        : instanceName(instanceName)
        , role(role) {
        instanceSc = utils::Config::addInstance(instanceName,
                                                std::string("Configuration for ")
                                                    .append(role == Role::SERVER ? "server" : "client")
                                                    .append(" instance '")
                                                    .append(instanceName)
                                                    .append("'"),
                                                "Instance");

        utils::Config::addStandardFlags(instanceSc);
        utils::Config::addHelp(instanceSc);

        disableOpt = instanceSc
                         ->add_flag_callback(
                             "--disabled{true}",
                             [this]() {
                                 utils::Config::disabled(instanceSc, disableOpt->as<bool>());
                             },
                             "Disable this instance")
                         ->trigger_on_parse()
                         ->default_val("false")
                         ->type_name("bool")
                         ->check(CLI::IsMember({"true", "false"}))
                         ->group(instanceSc->get_formatter()->get_label("Persistent Options"));
    }

    ConfigInstance::~ConfigInstance() {
        utils::Config::removeInstance(instanceSc);
    }

    ConfigInstance::Role ConfigInstance::getRole() {
        return role;
    }

    const std::string& ConfigInstance::getInstanceName() const {
        return instanceName.empty() ? nameAnonymous : instanceName;
    }

    void ConfigInstance::setInstanceName(const std::string& instanceName) {
        this->instanceName = instanceName;
    }

    CLI::App* ConfigInstance::addSection(const std::string& name, const std::string& description) {
        CLI::App* sectionSc = instanceSc //
                                  ->add_subcommand(name, description)
                                  ->fallthrough()
                                  ->configurable(false)
                                  ->allow_extras(false)
                                  ->group("Sections")
                                  ->disabled(this->instanceName.empty() || name.empty());

        sectionSc //
            ->option_defaults()
            ->configurable(!sectionSc->get_disabled());

        if (!sectionSc->get_disabled()) {
            utils::Config::addStandardFlags(sectionSc);
            utils::Config::addSimpleHelp(sectionSc);
        }

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

    CLI::App* ConfigInstance::getSection(const std::string& name) const {
        return instanceSc->get_subcommand_no_throw(name);
    }

    bool ConfigInstance::getDisabled() const {
        return disableOpt //
            ->as<bool>();
    }

    void ConfigInstance::setDisabled(bool disabled) {
        disableOpt //
            ->default_val(disabled ? "true" : "false")
            ->clear();

        utils::Config::disabled(instanceSc, disabled);
    }

} // namespace net::config
