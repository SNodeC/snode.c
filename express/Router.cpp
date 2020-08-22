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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Router.h"

namespace express {

    static inline std::string path_concat(const std::string& first, const std::string& second) {
        std::string result;

        if (first.back() == '/' && second.front() == '/') {
            result = first + second.substr(1);
        } else if (first.back() != '/' && second.front() != '/') {
            result = first + '/' + second;
        } else {
            result = first + second;
        }

        if (result.length() > 1 && result.back() == '/') {
            result.pop_back();
        }

        return result;
    }

    class Dispatcher {
    public:
        Dispatcher() = default;
        Dispatcher(const Dispatcher&) = delete;

        Dispatcher& operator=(const Dispatcher&) = delete;

        virtual ~Dispatcher() = default;

        [[nodiscard]] virtual bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, Request& req,
                                            Response& res) const = 0;
    };

    class Route {
    public:
        Route(Router* parent, const std::string& method, const std::string& path, const std::shared_ptr<Dispatcher>& dispatcher)
            : parent(parent)
            , mountPoint(method, path)
            , dispatcher(dispatcher) {
        }

        [[nodiscard]] bool dispatch(const std::string& parentPath, Request& req, Response& res) const;

    private:
        Router* parent;
        MountPoint mountPoint;
        std::shared_ptr<Dispatcher> dispatcher;
    };

    class RouterDispatcher : public Dispatcher {
    public:
        explicit RouterDispatcher() = default;

        [[nodiscard]] bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, Request& req,
                                    Response& res) const override;

    private:
        std::list<Route> routes;

        friend class Router;
    };

    class MiddlewareDispatcher : public Dispatcher {
    public:
        explicit MiddlewareDispatcher(
            const std::function<void(Request& req, Response& res, const std::function<void(void)>& next)>& dispatcher)
            : dispatcher(dispatcher) {
        }

        [[nodiscard]] bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, Request& req,
                                    Response& res) const override;

    private:
        const std::function<void(Request& req, Response& res, std::function<void(void)>)> dispatcher;
    };

    class ApplicationDispatcher : public Dispatcher {
    public:
        explicit ApplicationDispatcher(const std::function<void(Request& req, Response& res)>& dispatcher)
            : dispatcher(dispatcher) {
        }

        [[nodiscard]] bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, Request& req,
                                    Response& res) const override;

    protected:
        const std::function<void(Request& req, Response& res)> dispatcher;
    };

    bool Route::dispatch(const std::string& parentPath, Request& req, Response& res) const {
        return dispatcher->dispatch(mountPoint, parentPath, req, res);
    }

    bool RouterDispatcher::dispatch(const MountPoint& mountPoint, const std::string& parentPath, Request& req, Response& res) const {
        bool next = true;
        std::string cpath = path_concat(parentPath, mountPoint.path);

        if ((req.path.rfind(cpath, 0) == 0 &&
             (mountPoint.method == "use" || req.method == mountPoint.method || mountPoint.method == "all"))) {
            for (const Route& route : routes) {
                if (next) {
                    next = route.dispatch(cpath, req, res);
                } else {
                    break;
                }
            }
        }

        return next;
    }

    bool MiddlewareDispatcher::dispatch(const MountPoint& mountPoint, const std::string& parentPath, Request& req, Response& res) const {
        bool next = true;
        std::string cpath = path_concat(parentPath, mountPoint.path);

        if ((req.path.rfind(cpath, 0) == 0 && mountPoint.method == "use") ||
            (cpath == req.path && (req.method == mountPoint.method || mountPoint.method == "all"))) {
            next = false;
            this->dispatcher(req, res, [&next]() -> void {
                next = true;
            });
        }

        return next;
    }

    bool ApplicationDispatcher::dispatch(const MountPoint& mountPoint, const std::string& parentPath, Request& req, Response& res) const {
        bool next = true;
        std::string cpath = path_concat(parentPath, mountPoint.path);

        if ((req.path.rfind(cpath, 0) == 0 && mountPoint.method == "use") ||
            (cpath == req.path && (req.method == mountPoint.method || mountPoint.method == "all"))) {
            next = false;
            this->dispatcher(req, res);
        }

        return next;
    }

#define REQUESTMETHOD(METHOD, HTTP_METHOD)                                                                                                 \
    Router& Router::METHOD(const std::string& path, const Router& router) {                                                                \
        routerDispatcher->routes.emplace_back(Route(this, HTTP_METHOD, path, router.routerDispatcher));                                    \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& Router::METHOD(const Router& router) {                                                                                         \
        routerDispatcher->routes.emplace_back(Route(this, HTTP_METHOD, "", router.routerDispatcher));                                      \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& Router::METHOD(const std::string& path,                                                                                        \
                           const std::function<void(Request & req, Response & res, const std::function<void(void)>& next)>& dispatcher) {  \
        routerDispatcher->routes.emplace_back(Route(this, HTTP_METHOD, path, std::make_shared<MiddlewareDispatcher>(dispatcher)));         \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& Router::METHOD(const std::function<void(Request & req, Response & res, const std::function<void(void)>& next)>& dispatcher) {  \
        routerDispatcher->routes.emplace_back(Route(this, HTTP_METHOD, "", std::make_shared<MiddlewareDispatcher>(dispatcher)));           \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& Router::METHOD(const std::string& path, const std::function<void(Request & req, Response & res)>& dispatcher) {                \
        routerDispatcher->routes.emplace_back(Route(this, HTTP_METHOD, path, std::make_shared<ApplicationDispatcher>(dispatcher)));        \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& Router::METHOD(const std::function<void(Request & req, Response & res)>& dispatcher) {                                         \
        routerDispatcher->routes.emplace_back(Route(this, HTTP_METHOD, "", std::make_shared<ApplicationDispatcher>(dispatcher)));          \
        return *this;                                                                                                                      \
    };

    const MountPoint Router::mountPoint("use", "/");

    Router::Router()
        : routerDispatcher(new RouterDispatcher()) {
    }

    void Router::dispatch(Request& req, Response& res) const {
        [[maybe_unused]] bool next = routerDispatcher->dispatch(Router::mountPoint, "/", req, res);
    }

    REQUESTMETHOD(use, "use");
    REQUESTMETHOD(all, "all");
    REQUESTMETHOD(get, "GET");
    REQUESTMETHOD(put, "PUT");
    REQUESTMETHOD(post, "POST");
    REQUESTMETHOD(del, "DELETE");
    REQUESTMETHOD(connect, "CONNECT");
    REQUESTMETHOD(options, "OPTIONS");
    REQUESTMETHOD(trace, "TRACE");
    REQUESTMETHOD(patch, "PATCH");
    REQUESTMETHOD(head, "HEAD");

} // namespace express
