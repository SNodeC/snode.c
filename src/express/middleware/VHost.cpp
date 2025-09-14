/*
 * SNode.C - A Slim Toolkit for Network Communication
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

#include "express/middleware/VHost.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string_view>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::middleware {

    static std::string_view hostWithoutPort(std::string_view host) {
        if (host.empty()) {
            return host;
        }

        // Bracketed IPv6: "[::1]:8080" -> "::1"
        if (host.front() == '[') {
            if (auto rb = host.find(']'); rb != std::string_view::npos) {
                return host.substr(1, rb - 1);
            }

            return host; // malformed, just return as-is
        }

        // Find last ':'; if there is exactly one colon, treat it as host:port
        if (auto pos = host.rfind(':'); pos != std::string_view::npos) {
            if (host.find(':') == pos) { // only one colon -> host:port

                return host.substr(0, pos); // "localhost:8080" -> "localhost"
            }

            return host; // multiple colons (likely raw IPv6 without brackets) -> keep whole
        }

        return host; // no port
    }

    VHost::VHost(const std::string& host)
        : host(host) {
        use("/", [&host = this->host] MIDDLEWARE(req, res, next) {
            if (req->get("Host") == host || hostWithoutPort(req->get("Host")) == host) {
                next();
            } else {
                next("router");
            }
        });
    }

    class VHost& VHost::instance(const std::string& host) {
        // Keep all created vhost middlewares alive
        static std::map<std::string, std::shared_ptr<class VHost>> vHostMiddlewares;

        if (!vHostMiddlewares.contains(host)) {
            vHostMiddlewares[host] = std::shared_ptr<VHost>(new VHost(host));
        }

        return *vHostMiddlewares[host];
    }

    // "Constructor" of StaticMiddleware
    class VHost& VHost(const std::string& host) {
        return VHost::instance(host);
    }

} // namespace express::middleware
