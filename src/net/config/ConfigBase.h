/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#ifndef NET_CONFIG_CONFIGBASE_H
#define NET_CONFIG_CONFIGBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
} // namespace CLI

//#include "utils/CLI11.hpp"

#include <string> // IWYU pragma: export

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    class ConfigBase {
    public:
        ConfigBase() = default;
        explicit ConfigBase(const std::string& name);
        ConfigBase(const ConfigBase&) = delete;

        ConfigBase& operator=(const ConfigBase&) = delete;

        virtual ~ConfigBase() = default;

        const std::string& getName() const;

    protected:
        CLI::App* add_subcommand(const std::string& name, const std::string& description = "");
        CLI::Option* add_option(const std::string& name, int& variable, const std::string& description);
        CLI::Option* add_flag(const std::string& name, const std::string& description = "");

        void parse(bool forceError = false) const;

    private:
        CLI::App* baseSc = nullptr;

        const std::string name;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGBASE_H
