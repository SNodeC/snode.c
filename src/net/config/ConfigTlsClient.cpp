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

#include "net/config/ConfigTlsClient.h"

#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace net::config {

    ConfigTlsClient::ConfigTlsClient(ConfigInstance* instance)
        : ConfigTls(instance) {
        sniOpt = add_option(sniOpt, "--sni", "Server Name Indication", "sni", "", CLI::TypeValidator<std::string>());
    }

    std::string ConfigTlsClient::getSni() const {
        return sniOpt->as<std::string>();
    }

    void ConfigTlsClient::setSni(const std::string& sni) {
        sniOpt->default_val(sni)->clear();
    }

} // namespace net::config
