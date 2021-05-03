/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#include "VHost.h"

#include "express/Request.h"
#include "express/Response.h"
#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::middleware {

    VHost::VHost(const std::string& host)
        : host(host) {
        use([this] MIDDLEWARE(req, res, next) {
            VLOG(0) << "-------------------";
            std::string headerHost = req.header("Host");
            VLOG(0) << "Host: " << headerHost << " - " << this->host;

            if (headerHost == this->host) {
                VLOG(0) << "VHost dispatching";
                next();
            } else {
                VLOG(0) << "VHost not";
            }
        });
    }

    // Keep all created vhost middlewares alive
    static std::map<const std::string, std::shared_ptr<class VHost>> vhostMiddlewares;

    class VHost& VHost::instance(const std::string& host) {
        if (!vhostMiddlewares.contains(host)) {
            vhostMiddlewares[host] = std::shared_ptr<VHost>(new VHost(host));
        }

        return *vhostMiddlewares[host];
    }

    // "Constructor" of StaticMiddleware
    class VHost& VHost(const std::string& host) {
        return VHost::instance(host);
    }

} // namespace express::middleware
