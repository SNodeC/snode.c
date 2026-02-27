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

namespace net::config {
    class ConfigSection;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "net/config/ConfigInstanceAPI.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    const std::string ConfigInstance::nameAnonymous = "<anonymous>";

    ConfigInstance::ConfigInstance(const std::string& instanceName, Role role)
        : utils::SubCommand(utils::Config::newInstance(net::config::Instance(instanceName,
                                                                             std::string("Configuration for ")
                                                                                 .append(role == Role::SERVER ? "server" : "client")
                                                                                 .append(" instance '")
                                                                                 .append(instanceName)
                                                                                 .append("'"),
                                                                             this),
                                                       "Instances"))
        , instanceName(instanceName)
        , role(role) {
        disableOpt = subCommandSc
                         ->add_flag_function(
                             "--disabled{true}",
                             [this](std::size_t) {
                                 utils::Config::disabled(subCommandSc, disableOpt->as<bool>());
                             },
                             "Disable this instance")
                         ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast)
                         ->trigger_on_parse()
                         ->default_val("false")
                         ->type_name("bool")
                         ->check(CLI::IsMember({"true", "false"}))
                         ->group(subCommandSc->get_formatter()->get_label("Persistent Options"));
    }

    ConfigInstance::~ConfigInstance() {
        utils::Config::removeInstance(subCommandSc);
    }

    ConfigInstance::Role ConfigInstance::getRole() const {
        return role;
    }

    const std::string& ConfigInstance::getInstanceName() const {
        return instanceName.empty() ? nameAnonymous : instanceName;
    }

    void ConfigInstance::setInstanceName(const std::string& instanceName) {
        this->instanceName = instanceName;
    }

    CLI::App* ConfigInstance::newSection(std::shared_ptr<CLI::App> appWithPtr, const std::string& group) {
        CLI::App* sectionSc = subCommandSc->add_subcommand(appWithPtr)
                                  ->fallthrough()
                                  ->configurable(false)
                                  ->allow_extras()
                                  ->group(group)
                                  ->ignore_case(false)
                                  ->disabled(appWithPtr->get_name().empty())
                                  ->formatter(subCommandSc->get_formatter())
                                  ->config_formatter(subCommandSc->get_config_formatter());

        sectionSc //
            ->option_defaults()
            ->configurable(!sectionSc->get_disabled());

        if (!sectionSc->get_disabled()) {
            utils::Config::addStandardFlags(sectionSc);
            utils::Config::addSimpleHelp(sectionSc);
        }

        return sectionSc;
    }

    ConfigSection* ConfigInstance::addSection(std::shared_ptr<ConfigSection>&& configSection) {
        return configSections.emplace_back(configSection).get();
    }

    ConfigInstance& ConfigInstance::required(bool required) {
        if (!getDisabled()) {
            utils::Config::required(subCommandSc, required);
        }

        return *this;
    }

    ConfigInstance& ConfigInstance::required(CLI::App* section, bool req) {
        if (req != section->get_required()) {
            if (req) {
                ++requiredCount;
                subCommandSc //
                    ->needs(section);
            } else {
                --requiredCount;
                subCommandSc //
                    ->remove_needs(section);
            }

            section //
                ->required(req);
            section //
                ->ignore_case(req);

            required(requiredCount > 0);
        }

        return *this;
    }

    bool ConfigInstance::getRequired() const {
        return requiredCount > 0;
    }

    CLI::App* ConfigInstance::get() const {
        return subCommandSc;
    }

    ConfigInstance& ConfigInstance::configurable(bool configurable) {
        disableOpt->configurable(configurable);

        disableOpt->group(subCommandSc->get_formatter()->get_label(configurable ? "Persistent Options" : "Nonpersistent Options"));

        return *this;
    }

    const CLI::App* ConfigInstance::getSection(const std::string& name) const {
        return subCommandSc->get_subcommand_no_throw(name);
    }

    bool ConfigInstance::gotSection(const std::string& name) const {
        return subCommandSc //
            ->got_subcommand(name);
    }

    bool ConfigInstance::getDisabled() const {
        return disableOpt //
            ->as<bool>();
    }

    ConfigInstance& ConfigInstance::setDisabled(bool disabled) {
        disableOpt //
            ->default_val(disabled ? "true" : "false")
            ->clear();

        utils::Config::disabled(subCommandSc, disabled);

        return *this;
    }

} // namespace net::config
