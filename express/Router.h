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

#ifndef ROUTER_H
#define ROUTER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <map>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "Response.h"

#define MIDDLEWARE(req, res, next)                                                                                                         \
    ([[maybe_unused]] Request & (req), [[maybe_unused]] Response & (res), [[maybe_unused]] const std::function<void(void)>&(next))->void

#define APPLICATION(req, res) ([[maybe_unused]] Request & (req), [[maybe_unused]] Response & (res))->void

#define DECLARE_REQUESTMETHOD(METHOD)                                                                                                      \
    Router& METHOD(const std::string& path, const std::function<void(Request & req, Response & res)>& dispatcher);                         \
    Router& METHOD(const std::function<void(Request & req, Response & res)>& dispatcher);                                                  \
    Router& METHOD(const std::string& path, const Router& router);                                                                         \
    Router& METHOD(const Router& router);                                                                                                  \
    Router& METHOD(const std::string& path,                                                                                                \
                   const std::function<void(Request & req, Response & res, const std::function<void(void)>& next)>& dispatcher);           \
    Router& METHOD(const std::function<void(Request & req, Response & res, const std::function<void(void)>& next)>& dispatcher);

namespace express {

    struct MountPoint {
        MountPoint(const std::string& method, const std::string& path)
            : method(method)
            , path(path) {
        }

        std::string method;
        std::string path;
    };

    class RouterDispatcher;

    class Router {
    public:
        Router();
        Router(const Router& router);
        Router& operator=(const Router& router);

        DECLARE_REQUESTMETHOD(use);
        DECLARE_REQUESTMETHOD(all);
        DECLARE_REQUESTMETHOD(get);
        DECLARE_REQUESTMETHOD(put);
        DECLARE_REQUESTMETHOD(post);
        DECLARE_REQUESTMETHOD(del);
        DECLARE_REQUESTMETHOD(connect);
        DECLARE_REQUESTMETHOD(options);
        DECLARE_REQUESTMETHOD(trace);
        DECLARE_REQUESTMETHOD(patch);
        DECLARE_REQUESTMETHOD(head);

    protected:
        void dispatch(const http::Request& req, const http::Response& res);
        void completed(const http::Request& req, const http::Response& res);

    private:
        std::map<const http::Request*, Request*> reqestMap;
        std::map<const http::Response*, Response*> responseMap;
        std::shared_ptr<RouterDispatcher> routerDispatcher; // it can be shared by multiple routers
    };

} // namespace express

#endif // ROUTER_H
