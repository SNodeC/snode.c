/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021 Volker Christian <me@vchrist.at>
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
#include <list>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define MIDDLEWARE(req, res, state)                                                                                                        \
    ([[maybe_unused]] express::Request & (req), [[maybe_unused]] express::Response & (res), [[maybe_unused]] express::State & (state))

#define APPLICATION(req, res) ([[maybe_unused]] express::Request & (req), [[maybe_unused]] express::Response & (res))

namespace express {

    struct MountPoint;
    class Request;
    class Response;
    class RouterDispatcher;

    class State {
    public:
        void operator()(const std::string& how = "") {
            if (how == "route") {
                parentProceed = true;
            } else {
                proceed = true;
            }
        }

    private:
        bool proceed = true;
        bool parentProceed = false;

        friend class RouterDispatcher;
    };

    class Dispatcher {
        Dispatcher(const Dispatcher&) = delete;
        Dispatcher& operator=(const Dispatcher&) = delete;

    public:
        Dispatcher() = default;
        virtual ~Dispatcher() = default;

    private:
        virtual void dispatch(const RouterDispatcher* parentRouter,
                              const std::string& parentMountPath,
                              const MountPoint& mountPoint,
                              Request& req,
                              Response& res) const = 0;

        friend class Route;
    };

    class Route;

    class RouterDispatcher : public Dispatcher {
    public:
        void dispatch(Request& req, Response& res) const;

    private:
        void dispatch(const RouterDispatcher* parentRouter,
                      const std::string& parentMountPath,
                      const MountPoint& mountPoint,
                      Request& req,
                      Response& res) const override;

        void terminate() const {
            state.proceed = false;
        }

        State& getState() const {
            return state;
        }

        bool proceed() const {
            return state.proceed;
        }

        void returnTo(const RouterDispatcher* parentRouter) const;

        std::list<Route> routes;
        mutable State state;

        friend class Router;
        friend class ApplicationDispatcher;
        friend class MiddlewareDispatcher;
    };

    class Router {
    public:
        Router();
        Router(const Router& router);
        Router& operator=(const Router& router);

#define DECLARE_REQUESTMETHOD(METHOD)                                                                                                      \
    Router& METHOD(const std::string& relativeMountPath, const std::function<void(Request & req, Response & res)>& lambda);                \
    Router& METHOD(const std::function<void(Request & req, Response & res)>& lambda);                                                      \
    Router& METHOD(const std::string& relativeMountPath, const Router& router);                                                            \
    Router& METHOD(const Router& router);                                                                                                  \
    Router& METHOD(const std::string& relativeMountPath, const std::function<void(Request & req, Response & res, State & state)>& lambda); \
    Router& METHOD(const std::function<void(Request & req, Response & res, State & state)>& lambda);

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
        std::shared_ptr<RouterDispatcher> routerDispatcher; // it can be shared by multiple routers
    };

} // namespace express

#endif // EXPRESS_ROUTER_H
