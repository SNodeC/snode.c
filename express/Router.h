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

#ifndef EXPRESS_ROUTER_H
#define EXPRESS_ROUTER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <map>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MIDDLEWARE(req, res, next)                                                                                                         \
    ([[maybe_unused]] express::Request & (req), [[maybe_unused]] express::Response & (res), [[maybe_unused]] express::Next & (next))

#define APPLICATION(req, res) ([[maybe_unused]] express::Request & (req), [[maybe_unused]] express::Response & (res))

namespace http::server {
    class Request;
    class Response;
} // namespace http::server

namespace express {

    class Request;
    class Response;

    class Next {
    public:
        void operator()(const std::string& how = "") {
            if (how == "route") {
                nextRouter = true;
            } else {
                next = true;
            }
        }

        bool next = true;
        bool nextRouter = false;
    };

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

#define DECLARE_REQUESTMETHOD(METHOD)                                                                                                      \
    Router& METHOD(const std::string& path, const std::function<void(Request & req, Response & res)>& dispatcher);                         \
    Router& METHOD(const std::function<void(Request & req, Response & res)>& dispatcher);                                                  \
    Router& METHOD(const std::string& path, const Router& router);                                                                         \
    Router& METHOD(const Router& router);                                                                                                  \
    Router& METHOD(const std::string& path, const std::function<void(Request & req, Response & res, express::Next & next)>& dispatcher);   \
    Router& METHOD(const std::function<void(Request & req, Response & res, express::Next & next)>& dispatcher);

        DECLARE_REQUESTMETHOD(use)
        DECLARE_REQUESTMETHOD(all)
        DECLARE_REQUESTMETHOD(get)
        DECLARE_REQUESTMETHOD(put)
        DECLARE_REQUESTMETHOD(post)
        DECLARE_REQUESTMETHOD(del)
        DECLARE_REQUESTMETHOD(connect)
        DECLARE_REQUESTMETHOD(options)
        DECLARE_REQUESTMETHOD(trace)
        DECLARE_REQUESTMETHOD(patch)
        DECLARE_REQUESTMETHOD(head)

    protected:
        void dispatch(express::Request& req, express::Response& res);

    private:
        std::map<const http::server::Request*, Request*> reqestMap;
        std::map<const http::server::Response*, Response*> responseMap;
        std::shared_ptr<RouterDispatcher> routerDispatcher; // it can be shared by multiple routers
    };

} // namespace express

#endif // EXPRESS_ROUTER_H
