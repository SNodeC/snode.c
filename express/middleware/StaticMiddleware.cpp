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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <easylogging++.h>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "Response.h"
#include "StaticMiddleware.h"

using namespace express;

StaticMiddleware::StaticMiddleware(const std::string& root)
    : root(root) {
    use([] MIDDLEWARE(req, res, next) {
        if (req.method == "GET") {
            res.set("Connection", "Keep-Alive");
            res.cookie("CookieName", "CookieValue");
            next();
        } else {
            res.set("Connection", "Close").sendStatus(400);
        }
    });
    use([] MIDDLEWARE(req, res, next) {
        if (req.url == "/") {
            res.redirect(308, "/index.html");
        } else {
            next();
        }
    });
    use([this] APPLICATION(req, res) {
        VLOG(0) << "GET " + req.url + " -> " + this->root + req.url;
        res.sendFile(this->root + req.url, [&req](int ret) -> void {
            if (ret != 0) {
                PLOG(ERROR) << req.url;
            }
        });
    });
}

// Keep all created static middlewares alive
static std::map<const std::string, std::shared_ptr<class StaticMiddleware>> staticMiddlewares;

const class StaticMiddleware& StaticMiddleware::instance(const std::string& root) {
    if (!staticMiddlewares.contains(root)) {
        staticMiddlewares[root] = std::shared_ptr<StaticMiddleware>(new StaticMiddleware(root));
    }

    return *staticMiddlewares[root];
}

// "Constructor" of StaticMiddleware
const class StaticMiddleware& StaticMiddleware(const std::string& root) {
    return StaticMiddleware::instance(root);
}
