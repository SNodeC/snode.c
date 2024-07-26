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

#include "net/config/ConfigTlsClient.h"

#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    ConfigTlsClient::ConfigTlsClient(ConfigInstance* instance)
        : Super(instance) {
        sniOpt = addOption( //
            "--sni",
            "Server Name Indication",
            "sni",
            "",
            CLI::TypeValidator<std::string>());
    }

    ConfigTlsClient& ConfigTlsClient::setSni(const std::string& sni) {
        sniOpt //
            ->default_val(sni)
            ->clear();

        return *this;
    }

    std::string ConfigTlsClient::getSni() const {
        return sniOpt->as<std::string>();
    }

} // namespace net::config
