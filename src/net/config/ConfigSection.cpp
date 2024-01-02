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

#include "net/config/ConfigSection.h"

#include "net/config/ConfigInstance.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#ifdef __has_warning
#if __has_warning("-Wweak-vtables")
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#endif
#include "utils/CLI11.hpp" // IWYU pragma: export
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "log/Logger.h"
#include "utils/ResetToDefault.h"

#include <memory>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    ConfigSection::ConfigSection(ConfigInstance* instance, const std::string& name, const std::string& description)
        : instance(instance) {
        section = instance //
                      ->add_section(name, description)
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

    CLI::Option* ConfigSection::add_option(CLI::Option*& opt, const std::string& name, const std::string& description) {
        section->disabled(false);

        opt = section //
                  ->add_option_function<std::string>(name, utils::ResetToDefault(opt), description);

        if (opt->get_configurable()) {
            opt->group(section->get_formatter()->get_label("Persistent Options"));
        }

        return opt;
    }

    CLI::Option*
    ConfigSection::add_option(CLI::Option*& opt, const std::string& name, const std::string& description, const std::string& typeName) {
        return add_option(opt, name, description) //
            ->type_name(typeName);
    }

    CLI::Option* ConfigSection::add_option(CLI::Option*& opt,
                                           const std::string& name,
                                           const std::string& description,
                                           const std::string& typeName,
                                           const CLI::Validator& additionalValidator) {
        return add_option(opt, name, description, typeName) //
            ->check(additionalValidator);
    }

    CLI::Option*
    ConfigSection::add_flag(CLI::Option*& opt, const std::string& name, const std::string& description, const std::string& typeName) {
        section->disabled(false);

        opt = section //
                  ->add_flag_function(name, utils::ResetToDefault(opt), description)
                  ->type_name(typeName)
                  ->take_last();

        if (opt->get_configurable()) {
            opt->group(section->get_formatter()->get_label("Persistent Options"));
        }

        return opt;
    }

    CLI::Option* ConfigSection::add_flag(CLI::Option*& opt,
                                         const std::string& name,
                                         const std::string& description,
                                         const std::string& typeName,
                                         const CLI::Validator& additionalValidator) {
        return add_flag(opt, name, description, typeName) //
            ->check(additionalValidator);
    }

    CLI::Option* ConfigSection::add_flag_function(CLI::Option*& opt,
                                                  const std::string& name,
                                                  const std::function<void(int64_t)>& callback,
                                                  const std::string& description,
                                                  const std::string& typeName,
                                                  const std::string& defaultValue) {
        section->disabled(false);

        opt = section //
                  ->add_flag_function(name, callback, description)
                  ->take_last()
                  ->default_val(defaultValue)
                  ->type_name(typeName);

        if (opt->get_configurable()) {
            opt->group(section->get_formatter()->get_label("Persistent Options"));
        }

        return opt;
    }

    CLI::Option* ConfigSection::add_flag_function(CLI::Option*& opt,
                                                  const std::string& name,
                                                  const std::function<void(int64_t)>& callback,
                                                  const std::string& description,
                                                  const std::string& typeName,
                                                  const std::string& defaultValue,
                                                  const CLI::Validator& validator) {
        return add_flag_function(opt, name, callback, description, typeName, defaultValue) //
            ->check(validator);
    }

} // namespace net::config
