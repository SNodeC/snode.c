/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef WEB_HTTP_CLIENT_HTTPCONFIG_H
#define WEB_HTTP_CLIENT_HTTPCONFIG_H

#include "net/config/ConfigSection.h"

namespace net::config {
    class ConfigInstance;
} // namespace net::config

namespace CLI {
    class Option;
} // namespace CLI

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string_view>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::client {

    class ConfigHTTP : public net::config::ConfigSection {
    public:
        constexpr static std::string_view name{"http"};
        constexpr static std::string_view description{"HTTP behavior"};

        explicit ConfigHTTP(net::config::ConfigInstance* configInstance);

        ~ConfigHTTP() override;

        ConfigHTTP(ConfigHTTP&) = delete;
        ConfigHTTP& operator=(ConfigHTTP&) = delete;

        ConfigHTTP(ConfigHTTP&&) noexcept = delete;
        ConfigHTTP& operator=(ConfigHTTP&&) = delete;

        void setHostHeader(const std::string& hostHeader);
        std::string getHostHeader() const;

        void setPipelinedRequests(bool pipelinedRequests);
        bool getPipelinedRequests() const;

    private:
        CLI::Option* hostHeaderOpt = nullptr;
        CLI::Option* pipelinedRequestsOpt = nullptr;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_HTTPCONFIG_H
