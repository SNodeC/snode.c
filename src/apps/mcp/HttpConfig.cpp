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

#include "HttpConfig.h"

#include "Tools.h"
#include "utils/CLI11.hpp"

namespace apps::mcp {

    HttpConfig::HttpConfig(utils::SubCommand* parent, bool tls)
        : DevelopmentConfig(parent) {
        token = addOption("--demo-token", "Optional demo bearer token; empty disables auth", "token", std::string{}, CLI::Validator{});
        issuer =
            addOption("--authorization-server", "Optional HTTPS authorization server metadata URL", "url", std::string{}, CLI::Validator{});
        resource = addOption("--resource-uri",
                             "Canonical protected resource URI",
                             "uri",
                             tls ? std::string{"https://127.0.0.1:3338/mcp"} : std::string{"http://127.0.0.1:3337/mcp"},
                             CLI::Validator{});
        replay = addOption(
            "--replay-events", "Bounded replay event count; zero disables replay", "events", std::size_t{128}, CLI::Range(0, 4096));
    }

    HttpConfig::~HttpConfig() = default;

    std::string HttpConfig::getDemoToken() const {
        return token->as<std::string>();
    }

    std::string HttpConfig::getAuthorizationServer() const {
        return issuer->as<std::string>();
    }

    std::string HttpConfig::getResourceUri() const {
        return resource->as<std::string>();
    }

    std::size_t HttpConfig::getReplayEvents() const {
        return replay->as<std::size_t>();
    }

} // namespace apps::mcp
