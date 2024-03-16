/*
 * snode.c - a slim toolkit for network communication
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

#include "ConfigHTTP.h"

#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <memory>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#define XSTR(s) STR(s)
#define STR(s) #s

namespace web::http::client {

    ConfigHTTP::ConfigHTTP(net::config::ConfigInstance& configInstance) {
        configHttp = new net::config::ConfigSection(&configInstance, "http", "HTTP behavior");

        pipelinedRequestsOpt = configHttp->add_flag(pipelinedRequestsOpt,
                                                    "--pipelined-requests",
                                                    "Pipelined requests",
                                                    "bool",
                                                    XSTR(HTTP_REQUEST_PIPELINED),
                                                    CLI::IsMember({"true", "false"}));
    }

    ConfigHTTP::~ConfigHTTP() {
        delete configHttp;
    }

    void ConfigHTTP::setPipelinedRequests(bool pipelinedRequests) {
        pipelinedRequestsOpt->default_val(pipelinedRequests ? "true" : "false");
    }

    bool ConfigHTTP::getPipelinedRequests() const {
        return pipelinedRequestsOpt->as<bool>();
    }

} // namespace web::http::client
