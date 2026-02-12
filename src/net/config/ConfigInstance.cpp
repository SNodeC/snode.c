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

#include "net/config/ConfigInstanceAPI.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/Config.h"

#include <cstddef>
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
#if __has_warning("-Wmissing-noreturn")
#pragma GCC diagnostic ignored "-Wmissing-noreturn"
#endif
#if __has_warning("-Wnrvo")
#pragma GCC diagnostic ignored "-Wnrvo"
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
        auto configInstanceApp = net::config::Instance(instanceName,
                                                       std::string("Configuration for ")
                                                           .append(role == Role::SERVER ? "server" : "client")
                                                           .append(" instance '")
                                                           .append(instanceName)
                                                           .append("'"),
                                                       this);
        /*
                instanceSc = utils::Config::addInstance(instanceName,
                                                        std::string("Configuration for ")
                                                            .append(role == Role::SERVER ? "server" : "client")
                                                            .append(" instance '")
                                                            .append(instanceName)
                                                            .append("'"),
                                                        "Instances");
        */
        instanceSc = utils::Config::addInstance(configInstanceApp, "Instances");

        //        ConfigInstance* configInstance = utils::Config::getInstance<ConfigInstance>(instanceName);
        //        VLOG(0) << " ++++++++++++++++++++ From ConfigInstance" << configInstance->instanceName;

        disableOpt = instanceSc
                         ->add_flag_function(
                             "--disabled{true}",
                             [this](std::size_t) {
                                 utils::Config::disabled(instanceSc, disableOpt->as<bool>());
                             },
                             "Disable this instance")
                         ->multi_option_policy(CLI::MultiOptionPolicy::TakeLast)
                         ->trigger_on_parse()
                         ->default_str("false")
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

    CLI::App* ConfigInstance::addSection(const std::string& name, const std::string& description, const std::string& group) {
        CLI::App* sectionSc = instanceSc //
                                  ->add_subcommand(name, description)
                                  ->fallthrough()
                                  ->configurable(false)
                                  ->allow_extras(false)
                                  ->group(group)
                                  ->ignore_case(false)
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

    CLI::App* ConfigInstance::addSection(std::shared_ptr<CLI::App> appWithPtr, const std::string& group) {
        CLI::App* sectionSc = instanceSc->add_subcommand(appWithPtr)
                                  ->fallthrough()
                                  ->configurable(false)
                                  ->allow_extras(false)
                                  ->group(group)
                                  ->ignore_case(false)
                                  ->disabled(appWithPtr->get_name().empty());

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
                instanceSc //
                    ->needs(section);
            } else {
                --requiredCount;
                instanceSc //
                    ->remove_needs(section);
            }

            section //
                ->required(req);
            section //
                ->ignore_case(req);

            if (!getDisabled()) {
                utils::Config::required(instanceSc, requiredCount > 0);
            }
        }
    }

    bool ConfigInstance::getRequired() const {
        return requiredCount > 0;
    }

    CLI::App* ConfigInstance::get() const {
        return instanceSc;
    }

    void ConfigInstance::configurable(bool configurable) {
        disableOpt->configurable(configurable);

        disableOpt->group(instanceSc->get_formatter()->get_label(configurable ? "Persistent Options" : "Nonpersistent Options"));
    }

    CLI::App* ConfigInstance::getSection(const std::string& name, bool onlyGot, bool recursive) const {
        CLI::App* resultSc = nullptr;

        CLI::App* sectionSc = instanceSc->get_subcommand_no_throw(name);
        CLI::App* parentSectionSc = instanceSc->get_parent()->get_subcommand_no_throw(name);

        if (sectionSc != nullptr && (sectionSc->count_all() > 0 || !onlyGot)) {
            resultSc = sectionSc;
        } else if (recursive && parentSectionSc != nullptr && (parentSectionSc->count_all() > 0 || !onlyGot)) {
            resultSc = parentSectionSc;
        }

        return resultSc;
    }

    bool ConfigInstance::gotSection(const std::string& name, bool recursive) const {
        return instanceSc //
                   ->got_subcommand(name) ||
               (recursive && instanceSc->get_parent()->got_subcommand(name));
    }

    bool ConfigInstance::getDisabled() const {
        return disableOpt //
            ->as<bool>();
    }

    void ConfigInstance::setDisabled(bool disabled) {
        disableOpt //
            ->default_str(disabled ? "true" : "false")
            ->clear();

        utils::Config::disabled(instanceSc, disabled);
    }

} // namespace net::config
