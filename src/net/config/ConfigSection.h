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

#ifndef NET_CONFIG_CONFIGSECTION_H
#define NET_CONFIG_CONFIGSECTION_H

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

namespace CLI {
    class App;
    class Option;
    class Validator;
} // namespace CLI

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace net::config {

    class ConfigSection {
    public:
        ConfigSection(ConfigInstance* instance, const std::string& name, const std::string& description, bool hidden = false);

    protected:
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

        CLI::Option* add_flag(const std::string& name, const std::string& description);

        CLI::Option* add_flag(const std::string& name, const std::string& description, const CLI::Validator& additionalValidator);

        template <typename ValueTypeT>
        CLI::Option* add_flag(const std::string& name, const std::string& description, ValueTypeT defaultValue);

        template <typename ValueTypeT>
        CLI::Option* add_flag(const std::string& name,
                              const std::string& description,
                              ValueTypeT defaultValue,
                              const CLI::Validator& additionalValidator);

        void required(CLI::Option* opt, bool req = true);
        void required(bool req = true);

    private:
        ConfigInstance* instance = nullptr;

    protected:
        CLI::App* section = nullptr;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGSECTION_H