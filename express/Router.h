/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef ROUTER_H
#define ROUTER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "Response.h"

#define MIDDLEWARE(req, res, next)                                                                                                         \
    [&]([[maybe_unused]] Request & (req), [[maybe_unused]] Response & (res),                                                               \
        [[maybe_unused]] const std::function<void(void)>&(next)) -> void

#define APPLICATION(req, res) [&]([[maybe_unused]] Request & (req), [[maybe_unused]] Response & (res)) -> void

#define DREQUESTMETHOD(METHOD)                                                                                                             \
    Router& METHOD(const std::string& path, const std::function<void(Request & req, Response & res)>& dispatcher);                         \
    Router& METHOD(const std::function<void(Request & req, Response & res)>& dispatcher);                                                  \
    Router& METHOD(const std::string& path, const Router& router);                                                                         \
    Router& METHOD(const Router& router);                                                                                                  \
    Router& METHOD(const std::string& path,                                                                                                \
                   const std::function<void(Request & req, Response & res, const std::function<void(void)>& next)>& dispatcher);           \
    Router& METHOD(const std::function<void(Request & req, Response & res, const std::function<void(void)>& next)>& dispatcher);

class MountPoint {
private:
    MountPoint(const std::string& method, const std::string& path)
        : method(method)
        , path(path) {
    }

    std::string method;
    std::string path;

    friend class Route;
    friend class Router;
    friend class ApplicationDispatcher;
    friend class MiddlewareDispatcher;
    friend class RouterDispatcher;
};

class RouterDispatcher;

class Router {
public:
    Router();

    DREQUESTMETHOD(use);
    DREQUESTMETHOD(all);
    DREQUESTMETHOD(get);
    DREQUESTMETHOD(put);
    DREQUESTMETHOD(post);
    DREQUESTMETHOD(del);
    DREQUESTMETHOD(connect);
    DREQUESTMETHOD(options);
    DREQUESTMETHOD(trace);
    DREQUESTMETHOD(patch);
    DREQUESTMETHOD(head);

protected:
    void dispatch(Request& req, Response& res) const;

    const std::shared_ptr<RouterDispatcher>& getRoute() const {
        return routerDispatcher;
    }

private:
    static const MountPoint mountPoint;
    std::shared_ptr<RouterDispatcher> routerDispatcher; // it can be shared by multiple routers
};

#endif // ROUTER_H
