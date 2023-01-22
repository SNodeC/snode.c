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
#include "utils/ResetValidator.h"

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace net::config {

    ConfigTlsClient::ConfigTlsClient() {
        sniOpt = tlsSc                                               //
                     ->add_option("--sni", "Server Name Indication") //
                     ->type_name("sni")                              //
                     ->check(utils::ResetValidator(sniOpt));
    }

    std::string ConfigTlsClient::getSni() const {
        return sniOpt->as<std::string>();
    }

    void ConfigTlsClient::setSni(const std::string& sni) {
        sniOpt->default_val(sni)->clear();
    }

} // namespace net::config
