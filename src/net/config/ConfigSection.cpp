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

#include "utils/CLI11.hpp"
#include "utils/ResetToDefault.h"

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace net::config {

    ConfigSection::ConfigSection(ConfigInstance* instance, const std::string& name, const std::string& description, bool hidden)
        : instance(instance) {
        section = instance //
                      ->add_section(name, description);

        if (hidden) {
            section //
                ->group("");
        }
    }

    void ConfigSection::required(CLI::Option* opt) {
        opt //
            ->default_str("")
            ->required()
            ->clear();
        required();
    }

    void ConfigSection::required(bool req) {
        section //
            ->required(req)
            ->get_parent()
            ->required(req);
    }

    CLI::Option*
    ConfigSection::add_option(CLI::Option*& opt, const std::string& name, const std::string& description, const std::string& typeName) {
        opt = section //
                  ->add_option_function<std::string>(name, utils::ResetToDefault(opt), description)
                  ->type_name(typeName)
                  ->force_callback();

        return opt;
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
        return opt = section //
                         ->add_flag_function(name, utils::ResetToDefault(opt), description)
                         ->type_name(typeName)
                         ->take_last()
                         ->force_callback();
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
        opt = section //
                  ->add_flag_function(name, callback, description)
                  ->take_last()
                  ->default_val(defaultValue)
                  ->type_name(typeName);

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

    CLI::Option* ConfigSection::add_flag_callback(CLI::Option*& opt,
                                                  const std::string& name,
                                                  const std::function<void()>& callback,
                                                  const std::string& description,
                                                  const std::string& typeName,
                                                  const std::string& defaultValue) {
        opt = section //
                  ->add_flag_callback(name, callback, description)
                  ->default_val(defaultValue)
                  ->type_name(typeName);

        return opt;
    }

    CLI::Option* ConfigSection::add_flag_callback(CLI::Option*& opt,
                                                  const std::string& name,
                                                  const std::function<void()>& callback,
                                                  const std::string& description,
                                                  const std::string& typeName,
                                                  const std::string& defaultValue,
                                                  const CLI::Validator& validator) {
        return add_flag_callback(opt, name, callback, description, typeName, defaultValue) //
            ->check(validator);
    }

} // namespace net::config
