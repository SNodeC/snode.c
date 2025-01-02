/*
 * snode.c - a slim toolkit for network communication
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

#ifndef WEB_HTTP_CLIENT_HTTPCONFIG_H
#define WEB_HTTP_CLIENT_HTTPCONFIG_H

namespace net::config {
    class ConfigInstance;
} // namespace net::config

namespace CLI {
    class Option;
} // namespace CLI

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::client {

    class ConfigHTTP {
    public:
        explicit ConfigHTTP(net::config::ConfigInstance& configInstance);
        ConfigHTTP(ConfigHTTP&) = delete;
        ConfigHTTP& operator=(ConfigHTTP&) = delete;

        ConfigHTTP(ConfigHTTP&&) noexcept = default;
        ConfigHTTP& operator=(ConfigHTTP&&) = delete;

        void setPipelinedRequests(bool pipelinedRequests);

        bool getPipelinedRequests() const;

    private:
        CLI::Option* pipelinedRequestsOpt = nullptr;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_HTTPCONFIG_H
