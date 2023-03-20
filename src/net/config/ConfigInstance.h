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

#ifndef NET_CONFIG_CONFIGINSTANCE_H
#define NET_CONFIG_CONFIGINSTANCE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
} // namespace CLI

#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    class ConfigInstance {
    public:
        ConfigInstance() = default;
        explicit ConfigInstance(const std::string& name, const std::string& description);
        ConfigInstance(const ConfigInstance&) = delete;
        ConfigInstance(ConfigInstance&&) = delete;

        ConfigInstance& operator=(const ConfigInstance&) = delete;
        ConfigInstance& operator=(ConfigInstance&&) = delete;

        virtual ~ConfigInstance();

        const std::string& getInstanceName() const;

        bool getDisabled() const;
        void setDisabled(bool disabled = true);

        CLI::App* add_section(const std::string& name, const std::string& description);

        void required(CLI::App* section, CLI::Option* opt, bool req = true);

        bool required() const;

    private:
        uint8_t requiredCount = 0;

        const std::string name;

        CLI::App* instanceSc = nullptr;
        CLI::Option* disableOpt = nullptr;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGINSTANCE_H
