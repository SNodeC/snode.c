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

#include "net/config/ConfigSection.h" // IWYU pragma: export

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
#if __has_warning("-Wunsafe-buffer-usage")
#pragma GCC diagnostic ignored "-Wunsafe-buffer-usage"
#endif
#endif
#include "utils/CLI11.hpp" // IWYU pragma: export
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    template <typename ValueType>
    CLI::Option*
    ConfigSection::addOption(const std::string& name, const std::string& description, const std::string& typeName, ValueType defaultValue) {
        return addOption(name, description, typeName) //
            ->default_val(defaultValue);
    }

    template <typename ValueType>
    CLI::Option* ConfigSection::addOption(const std::string& name,
                                          const std::string& description,
                                          const std::string& typeName,
                                          ValueType defaultValue,
                                          const CLI::Validator& additionalValidator) {
        return addOption(name, description, typeName, defaultValue) //
            ->check(additionalValidator);
    }

    template <typename ValueType>
    CLI::Option*
    ConfigSection::addFlag(const std::string& name, const std::string& description, const std::string& typeName, ValueType defaultValue) {
        return addFlag(name, description, typeName) //
            ->default_val(defaultValue);
    }

    template <typename ValueType>
    CLI::Option* ConfigSection::addFlag(const std::string& name,
                                        const std::string& description,
                                        const std::string& typeName,
                                        ValueType defaultValue,
                                        const CLI::Validator& additionalValidator) {
        return addFlag(name, description, typeName, defaultValue) //
            ->check(additionalValidator);
    }

} // namespace net::config
