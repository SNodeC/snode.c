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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace net::config {

    ConfigTlsClient::ConfigTlsClient() {
        if (!getName().empty()) {
            sniOpt = tlsSc->add_option("--sni", sni, "Server Name Indication");
            sniOpt->type_name("sni");
        }
    }

    std::string ConfigTlsClient::getSni() const {
        return sni;
    }

    void ConfigTlsClient::setSni(const std::string& sni) {
        this->sni = sni;
    }

} // namespace net::config
