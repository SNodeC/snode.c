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

#ifndef NET_CONFIG_CONFIGTLSSERVER_H
#define NET_CONFIG_CONFIGTLSSERVER_H

#include "net/config/ConfigTls.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <map>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace net::config {

    class ConfigTlsServer : public ConfigTls {
    public:
        ConfigTlsServer();

        bool getForceSni() const;
        void setForceSni(bool forceSni = true);

        std::map<std::string, std::map<std::string, std::any>>& getSniCerts();
        void setSniCerts(const std::map<std::string, std::map<std::string, std::any>>& newSniCerts);

    private:
        bool forceSni = false;
        std::map<std::string, std::map<std::string, std::any>> sniCerts;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGTLSSERVER_H
