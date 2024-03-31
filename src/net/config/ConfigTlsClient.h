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

#ifndef NET_CONFIG_CONFIGTLSCLIENT_H
#define NET_CONFIG_CONFIGTLSCLIENT_H

#include "net/config/ConfigTls.h" // IWYU pragma: export

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>

namespace CLI {
    class Option;
}

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    class ConfigTlsClient : public ConfigTls {
    public:
        using Tls = ConfigTlsClient;

        explicit ConfigTlsClient(ConfigInstance* instance);

        ConfigTlsClient& setSni(const std::string& sni);
        std::string getSni() const;

    private:
        CLI::Option* sniOpt = nullptr;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGTLSCLIENT_H
