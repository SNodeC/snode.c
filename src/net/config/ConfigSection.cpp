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
#include "utils/ResetValidator.h"

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace net::config {

    ConfigSection::ConfigSection(ConfigInstance* instance, const std::string& name, const std::string& description, bool hidden)
        : instance(instance) {
        section = instance->add_section(name, description);

        if (hidden) {
            section->group("");
        }
    }

    void ConfigSection::required(CLI::Option* opt, bool req) {
        opt->default_str("")->required()->clear();
        required(req);
    }

    void ConfigSection::required(bool req) {
        section //
            ->required(req)
            ->get_parent()
            ->required(req);
    }

    CLI::Option* ConfigSection::add_option(const std::string& name, const std::string& description, const std::string& typeName) {
        CLI::Option* option = section->add_option(name, description);

        if (!typeName.empty()) {
            option //
                ->type_name(typeName);
        }
        option //
            ->check(utils::ResetValidator(option));

        return option;
    }

    CLI::Option* ConfigSection::add_option(const std::string& name,
                                           const std::string& description,
                                           const std::string& typeName,
                                           const CLI::Validator& additionalValidator) {
        return add_option(name, description, typeName)->check(additionalValidator);
    }

    CLI::Option* ConfigSection::add_flag(const std::string& name, const std::string& description) {
        CLI::Option* flag = section->add_flag(name, description);
        return flag //
            ->check(utils::ResetValidator(flag));
    }

    CLI::Option*
    ConfigSection::add_flag(const std::string& name, const std::string& description, const CLI::Validator& additionalValidator) {
        return add_flag(name, description)->check(additionalValidator);
    }

} // namespace net::config
