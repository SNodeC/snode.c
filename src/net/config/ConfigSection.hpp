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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace net::config {

    template <typename ValueType>
    CLI::Option* ConfigSection::add_option(const std::string& name,
                                           const std::string& description,
                                           const std::string& typeName,
                                           ValueType defaultValue) {
        CLI::Option* option = add_option(name, description, typeName);
        return option->default_val(defaultValue);
    }

    template <typename ValueType>
    CLI::Option* ConfigSection::add_option(const std::string& name,
                                           const std::string& description,
                                           const std::string& typeName,
                                           ValueType defaultValue,
                                           const CLI::Validator& additionalValidator) {
        return add_option(name, description, typeName, defaultValue)->check(additionalValidator);
    }

    template <typename ValueType>
    CLI::Option* ConfigSection::add_flag(const std::string& name, const std::string& description, ValueType defaultValue) {
        return add_flag(name, description)->default_val(defaultValue);
    }

    template <typename ValueType>
    CLI::Option* ConfigSection::add_flag(const std::string& name,
                                         const std::string& description,
                                         ValueType defaultValue,
                                         const CLI::Validator& additionalValidator) {
        return add_flag(name, description, defaultValue)->check(additionalValidator);
    }

} // namespace net::config
