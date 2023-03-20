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

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif
#include "utils/CLI11.hpp" // IWYU pragma: export
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace net::config {

    template <typename ValueType>
    CLI::Option* ConfigSection::add_option(
        CLI::Option*& opt, const std::string& name, const std::string& description, const std::string& typeName, ValueType defaultValue) {
        return add_option(opt, name, description, typeName) //
            ->default_val(defaultValue);
    }

    template <typename ValueType>
    CLI::Option* ConfigSection::add_option(CLI::Option*& opt,
                                           const std::string& name,
                                           const std::string& description,
                                           const std::string& typeName,
                                           ValueType defaultValue,
                                           const CLI::Validator& additionalValidator) {
        return add_option(opt, name, description, typeName, defaultValue) //
            ->check(additionalValidator);
    }

    template <typename ValueType>
    CLI::Option* ConfigSection::add_flag(
        CLI::Option*& opt, const std::string& name, const std::string& description, const std::string& typeName, ValueType defaultValue) {
        return add_flag(opt, name, description, typeName) //
            ->default_val(defaultValue);
    }

    template <typename ValueType>
    CLI::Option* ConfigSection::add_flag(CLI::Option*& opt,
                                         const std::string& name,
                                         const std::string& description,
                                         const std::string& typeName,
                                         ValueType defaultValue,
                                         const CLI::Validator& additionalValidator) {
        return add_flag(opt, name, description, typeName, defaultValue) //
            ->check(additionalValidator);
    }

} // namespace net::config
