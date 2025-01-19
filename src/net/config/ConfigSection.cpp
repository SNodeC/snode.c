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

#include "net/config/ConfigSection.h"

#include "net/config/ConfigInstance.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __has_warning
#if __has_warning("-Wweak-vtables")
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#endif
#include "utils/CLI11.hpp"
#pragma GCC diagnostic pop
#endif

#include <memory>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    ConfigSection::ConfigSection(ConfigInstance* instance, const std::string& name, const std::string& description)
        : instance(instance) {
        CLI::App* existingSection = instance->getSection(name);
        section = existingSection != nullptr ? existingSection
                                             : instance //
                                                   ->addSection(name, description + " for instance '" + instance->getInstanceName() + "'")
                                                   ->disabled()
                                                   ->group("Sections");
    }

    void ConfigSection::required(CLI::Option* opt, bool req) {
        if (req != opt->get_required()) {
            if (req) {
                ++requiredCount;
                section->needs(opt);
                opt->default_str("");
            } else {
                --requiredCount;
                section->remove_needs(opt);
            }

            opt->required(req);

            instance->required(section, requiredCount > 0);
        }
    }

    bool ConfigSection::required() const {
        return requiredCount > 0;
    }

    CLI::Option* ConfigSection::addOption(const std::string& name, const std::string& description) {
        if (!instance->getInstanceName().empty() && !section->get_display_name().empty()) {
            section->disabled(false);
        }

        CLI::Option* opt = section //
                               ->add_option(name, description);

        if (opt->get_configurable()) {
            opt->group(section->get_formatter()->get_label("Persistent Options"));
        }

        return opt;
    }

    CLI::Option* ConfigSection::addOption(const std::string& name, const std::string& description, const std::string& typeName) {
        return addOption(name, description) //
            ->type_name(typeName);
    }

    CLI::Option* ConfigSection::addOption(const std::string& name,
                                          const std::string& description,
                                          const std::string& typeName,
                                          const CLI::Validator& additionalValidator) {
        return addOption(name, description, typeName) //
            ->check(additionalValidator);
    }

    CLI::Option* ConfigSection::addFlag(const std::string& name, const std::string& description, const std::string& typeName) {
        section->disabled(false);

        CLI::Option* opt = section //
                               ->add_flag(name, description)
                               ->type_name(typeName);
        if (opt->get_configurable()) {
            opt->group(section->get_formatter()->get_label("Persistent Options"));
        }

        return opt;
    }

    CLI::Option* ConfigSection::addFlag(const std::string& name,
                                        const std::string& description,
                                        const std::string& typeName,
                                        const CLI::Validator& additionalValidator) {
        return addFlag(name, description, typeName) //
            ->check(additionalValidator);
    }

    CLI::Option* ConfigSection::addFlagFunction(const std::string& name,
                                                const std::function<void()>& callback,
                                                const std::string& description,
                                                const std::string& typeName,
                                                const std::string& defaultValue) {
        section->disabled(false);

        CLI::Option* opt = section //
                               ->add_flag_callback(name, callback, description)
                               ->default_val(defaultValue)
                               ->type_name(typeName);
        if (opt->get_configurable()) {
            opt->group(section->get_formatter()->get_label("Persistent Options"));
        }

        return opt;
    }

    CLI::Option* ConfigSection::addFlagFunction(const std::string& name,
                                                const std::function<void()>& callback,
                                                const std::string& description,
                                                const std::string& typeName,
                                                const std::string& defaultValue,
                                                const CLI::Validator& validator) {
        return addFlagFunction(name, callback, description, typeName, defaultValue) //
            ->check(validator);
    }

} // namespace net::config
