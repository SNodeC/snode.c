/*
 * SNode.C - a slim toolkit for network communication
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

#include "express/middleware/VHost.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::middleware {

    VHost::VHost(const std::string& host)
        : host(host) {
        use([&host = this->host] MIDDLEWARE(req, res, next) {
            if (req->get("Host") == host) {
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
