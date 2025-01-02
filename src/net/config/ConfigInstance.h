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

#ifndef NET_CONFIG_CONFIGINSTANCE_H
#define NET_CONFIG_CONFIGINSTANCE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
} // namespace CLI

namespace net::config {
    class ConfigSection;
}

#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    class ConfigInstance {
    public:
        using Instance = ConfigInstance;

        enum class Role { SERVER, CLIENT };

    protected:
        explicit ConfigInstance(const std::string& instanceName, Role role);

        virtual ~ConfigInstance();

    public:
        ConfigInstance(ConfigInstance&) = delete;
        ConfigInstance(ConfigInstance&&) = delete;

        ConfigInstance& operator=(ConfigInstance&) = delete;
        ConfigInstance& operator=(ConfigInstance&&) = delete;

        Role getRole();

        const std::string& getInstanceName() const;
        void setInstanceName(const std::string& instanceName);

        bool getDisabled() const;
        void setDisabled(bool disabled = true);

    private:
        CLI::App* addSection(const std::string& name, const std::string& description);

        CLI::App* getSection(const std::string& name) const;

        void required(CLI::App* section, bool req = true);

        bool getRequired() const;

        uint8_t requiredCount = 0;

        std::string instanceName;
        static const std::string nameAnonymous;

        Role role;

        CLI::App* instanceSc = nullptr;
        CLI::Option* disableOpt = nullptr;

        friend class net::config::ConfigSection;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGINSTANCE_H
