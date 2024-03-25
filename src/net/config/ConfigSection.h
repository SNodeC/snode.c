/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#ifndef NET_CONFIG_CONFIGSECTION_H
#define NET_CONFIG_CONFIGSECTION_H

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <functional>
#include <string>

namespace CLI {
    class App;
    class Option;
    class Validator;
} // namespace CLI

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    class ConfigSection {
    public:
        ConfigSection(ConfigInstance* instance, const std::string& name, const std::string& description);
        ConfigSection(const ConfigSection&) = delete;
        ConfigSection(ConfigSection&&) = delete;

        ConfigSection& operator=(const ConfigSection&) = delete;
        ConfigSection& operator=(ConfigSection&&) = delete;

        CLI::Option* add_option(const std::string& name, const std::string& description);

        CLI::Option* add_option(const std::string& name, const std::string& description, const std::string& typeName);

        CLI::Option* add_option(const std::string& name,
                                const std::string& description,
                                const std::string& typeName,
                                const CLI::Validator& additionalValidator);

        template <typename ValueTypeT>
        CLI::Option*
        add_option(const std::string& name, const std::string& description, const std::string& typeName, ValueTypeT defaultValue);

        template <typename ValueTypeT>
        CLI::Option* add_option(const std::string& name,
                                const std::string& description,
                                const std::string& typeName,
                                ValueTypeT defaultValue,
                                const CLI::Validator& additionalValidator);

        CLI::Option* add_flag(const std::string& name, const std::string& description, const std::string& typeName);

        CLI::Option* add_flag(const std::string& name,
                              const std::string& description,
                              const std::string& typeName,
                              const CLI::Validator& additionalValidator);

        template <typename ValueTypeT>
        CLI::Option*
        add_flag(const std::string& name, const std::string& description, const std::string& typeName, ValueTypeT defaultValue);

        template <typename ValueTypeT>
        CLI::Option* add_flag(const std::string& name,
                              const std::string& description,
                              const std::string& typeName,
                              ValueTypeT defaultValue,
                              const CLI::Validator& additionalValidator);

        CLI::Option* add_flag_function(const std::string& name,
                                       const std::function<void(int64_t)>& callback,
                                       const std::string& description,
                                       const std::string& typeName,
                                       const std::string& defaultValue);

        CLI::Option* add_flag_function(const std::string& name,
                                       const std::function<void(int64_t)>& callback,
                                       const std::string& description,
                                       const std::string& typeName,
                                       const std::string& defaultValue,
                                       const CLI::Validator& validator);

        void required(CLI::Option* opt, bool req = true);

        bool required() const;

    private:
        ConfigInstance* instance = nullptr;

        uint8_t requiredCount = 0;

    protected:
        CLI::App* section = nullptr;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGSECTION_H
