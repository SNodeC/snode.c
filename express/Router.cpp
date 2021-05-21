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

#include "express/Router.h"

#include "express/Request.h"
#include "web/http_utils.h"
#include "web/server/http/HTTPServerContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <filesystem>
#include <list>
#include <regex>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express {

#define PATH_REGEX ":[a-zA-Z0-9]+(\\(.+?\\))?"

    static std::regex pathregex = std::regex(PATH_REGEX);

    static const std::string path_concat(const std::vector<std::string>& stringvec) {
        std::string s;
        for (std::vector<std::string>::size_type i = 0; i < stringvec.size(); i++) {
            if (!stringvec[i].empty() && stringvec[i].front() != ' ') {
                s += "\\/" + stringvec[i];
            }
        }
        return s;
    }

    static const std::vector<std::string> explode(const std::string& s, char delim) {
        std::vector<std::string> result;
        std::istringstream iss(s);

        for (std::string token; std::getline(iss, token, delim);) {
            result.push_back(std::move(token));
        }

        return result;
    }

    static const std::smatch matchResult(const std::string& cpath) {
        std::smatch smatch;

        std::regex_search(cpath, smatch, pathregex);

        return smatch;
    }

    static bool hasResult(const std::string& cpath) {
        std::smatch smatch;

        return std::regex_search(cpath, smatch, pathregex);
    }

    static bool matchFunction(const std::string& cpath, const std::string& reqpath) {
        std::vector<std::string> explodedString = explode(cpath, '/');

        for (std::vector<std::string>::size_type i = 0; i < explodedString.size(); i++) {
            if (explodedString[i].front() == ':') {
                std::smatch smatch = matchResult(explodedString[i]);
                std::string regex = "(.*)";

                if (smatch.size() > 1) {
                    if (smatch[1] != "") {
                        regex = smatch[1];
                    }
                }

                explodedString[i] = regex;
            }
        }

        std::string regexPath = path_concat(explodedString);

        return std::regex_match(reqpath, std::regex(regexPath));
    }

    static void setParams(const std::string& cpath, Request& req) {
        std::vector<std::string> explodedString = explode(cpath, '/');
        std::vector<std::string> explodedReqString = explode(req.url, '/');

        for (std::vector<std::string>::size_type i = 0; i < explodedString.size() && i < explodedReqString.size(); i++) {
            if (explodedString[i].front() == ':') {
                std::smatch smatch = matchResult(explodedString[i]);
                std::string regex = "(.*)";

                if (smatch.size() > 1) {
                    if (smatch[1] != "") {
                        regex = smatch[1];
                    }
                }

                if (std::regex_match(explodedReqString[i], std::regex(regex))) {
                    std::string attributeName = smatch[0];
                    attributeName.erase(0, 1);
                    attributeName.erase((attributeName.length() - smatch[1].length()), smatch[1].length());

                    req.params[attributeName] = explodedReqString[i];
                }
            }
        }
    }

    static bool checkForUrlMatch(const std::string& cpath, const std::string& reqpath) {
        bool hasRegex = hasResult(cpath);
        return (!hasRegex && cpath == reqpath) || (hasRegex && matchFunction(cpath, reqpath));
    }

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

    struct MountPoint {
        MountPoint(const std::string& method, const std::string& relativeMountPath)
            : method(method)
            , relativeMountPath(relativeMountPath) {
        }

        std::string method;
        std::string relativeMountPath;
    };

    class Route {
    public:
        Route(RouterDispatcher* parentRouter,
              const std::string& method,
              const std::string& relativeMountPath,
              const std::shared_ptr<Dispatcher>& dispatcher)
            : parentRouter(parentRouter)
            , mountPoint(method, relativeMountPath)
            , dispatcher(dispatcher) {
        }

        void dispatch(const RouterDispatcher* parentRouter, const std::string& parentMountPath, Request& req, Response& res) const;

    protected:
        RouterDispatcher* parentRouter;
        MountPoint mountPoint;
        std::shared_ptr<Dispatcher> dispatcher;

        friend RouterDispatcher;
    };

    class MiddlewareDispatcher : public Dispatcher {
    public:
        explicit MiddlewareDispatcher(const std::function<void(Request& req, Response& res, State& state)>& lambda)
            : lambda(lambda) {
        }

    private:
        void dispatch(const RouterDispatcher* parentRouter,
                      const std::string& parentMountPath,
                      const MountPoint& mountPoint,
                      Request& req,
                      Response& res) const override;

        const std::function<void(Request& req, Response& res, State& state)> lambda;
    };

    class ApplicationDispatcher : public Dispatcher {
    public:
        explicit ApplicationDispatcher(const std::function<void(Request& req, Response& res)>& lambda)
            : lambda(lambda) {
        }

    private:
        void dispatch(const RouterDispatcher* parentRouter,
                      const std::string& parentMountPath,
                      const MountPoint& mountPoint,
                      Request& req,
                      Response& res) const override;

        const std::function<void(Request& req, Response& res)> lambda;
    };

    void Route::dispatch(const RouterDispatcher* parentRouter, const std::string& parentMountPath, Request& req, Response& res) const {
        return dispatcher->dispatch(parentRouter, parentMountPath, mountPoint, req, res);
    }

    void RouterDispatcher::dispatch(const RouterDispatcher* parentRouter,
                                    const std::string& parentMountPath,
                                    const MountPoint& mountPoint,
                                    Request& req,
                                    Response& res) const {
        std::string absoluteMountPath = path_concat(parentMountPath, mountPoint.relativeMountPath);

        // TODO: Fix regex-match
        if ((req.path.rfind(absoluteMountPath, 0) == 0 &&
             (mountPoint.method == "use" || req.method == mountPoint.method || mountPoint.method == "all"))) {
            for (const Route& route : routes) {
                route.dispatch(this, absoluteMountPath, req, res);

                if (!proceed()) {
                    break;
                }
            }
        }

        returnTo(parentRouter);
    }

    void RouterDispatcher::dispatch(Request& req, Response& res) const {
        dispatch(nullptr, "/", MountPoint("use", "/"), req, res);
    }

    void RouterDispatcher::returnTo(const RouterDispatcher* parentRouter) const {
        if (parentRouter) {
            parentRouter->state.proceed = state.proceed | state.parentProceed;
        }

        state.proceed = true;
        state.parentProceed = false;
    }

    void MiddlewareDispatcher::dispatch(const RouterDispatcher* parentRouter,
                                        const std::string& parentMountPath,
                                        const MountPoint& mountPoint,
                                        Request& req,
                                        Response& res) const {
        std::string absoluteMountPath = path_concat(parentMountPath, mountPoint.relativeMountPath);

        // TODO: Fix regex-match
        if ((req.path.rfind(absoluteMountPath, 0) == 0 && mountPoint.method == "use") ||
            ((absoluteMountPath == req.path || checkForUrlMatch(absoluteMountPath, req.url)) &&
             (req.method == mountPoint.method || mountPoint.method == "all"))) {
            parentRouter->terminate();

            if (hasResult(absoluteMountPath)) {
                setParams(absoluteMountPath, req);
            }

            lambda(req, res, parentRouter->getState());
        }
    }

    void ApplicationDispatcher::dispatch(const RouterDispatcher* parentRouter,
                                         const std::string& parentMountPath,
                                         const MountPoint& mountPoint,
                                         Request& req,
                                         Response& res) const {
        std::string absoluteMountPath = path_concat(parentMountPath, mountPoint.relativeMountPath);

        // TODO: Fix regex-match
        if ((req.path.rfind(absoluteMountPath, 0) == 0 && mountPoint.method == "use") ||
            ((absoluteMountPath == req.path || checkForUrlMatch(absoluteMountPath, req.url)) &&
             (req.method == mountPoint.method || mountPoint.method == "all"))) {
            parentRouter->terminate();

            if (hasResult(absoluteMountPath)) {
                setParams(absoluteMountPath, req);
            }

            lambda(req, res);
        }
    }

    Router::Router()
        : routerDispatcher(new RouterDispatcher()) {
    }

    Router::Router(const Router& router)
        : routerDispatcher(router.routerDispatcher) {
    }

    Router& Router::operator=(const Router& router) {
        routerDispatcher = router.routerDispatcher;
        return *this;
    }

#define DEFINE_REQUESTMETHOD(METHOD, HTTP_METHOD)                                                                                          \
    Router& Router::METHOD(const std::string& relativeMountPath, const Router& router) {                                                   \
        routerDispatcher->routes.emplace_back(Route(routerDispatcher.get(), HTTP_METHOD, relativeMountPath, router.routerDispatcher));     \
        return *this;                                                                                                                      \
    }                                                                                                                                      \
    Router& Router::METHOD(const Router& router) {                                                                                         \
        routerDispatcher->routes.emplace_back(Route(routerDispatcher.get(), HTTP_METHOD, "/", router.routerDispatcher));                   \
        return *this;                                                                                                                      \
    }                                                                                                                                      \
    Router& Router::METHOD(const std::string& relativeMountPath,                                                                           \
                           const std::function<void(Request & req, Response & res, State & state)>& lambda) {                              \
        routerDispatcher->routes.emplace_back(                                                                                             \
            Route(routerDispatcher.get(), HTTP_METHOD, relativeMountPath, std::make_shared<MiddlewareDispatcher>(lambda)));                \
        return *this;                                                                                                                      \
    }                                                                                                                                      \
    Router& Router::METHOD(const std::function<void(Request & req, Response & res, State & state)>& lambda) {                              \
        routerDispatcher->routes.emplace_back(                                                                                             \
            Route(routerDispatcher.get(), HTTP_METHOD, "/", std::make_shared<MiddlewareDispatcher>(lambda)));                              \
        return *this;                                                                                                                      \
    }                                                                                                                                      \
    Router& Router::METHOD(const std::string& relativeMountPath, const std::function<void(Request & req, Response & res)>& lambda) {       \
        routerDispatcher->routes.emplace_back(                                                                                             \
            Route(routerDispatcher.get(), HTTP_METHOD, relativeMountPath, std::make_shared<ApplicationDispatcher>(lambda)));               \
        return *this;                                                                                                                      \
    }                                                                                                                                      \
    Router& Router::METHOD(const std::function<void(Request & req, Response & res)>& lambda) {                                             \
        routerDispatcher->routes.emplace_back(                                                                                             \
            Route(routerDispatcher.get(), HTTP_METHOD, "/", std::make_shared<ApplicationDispatcher>(lambda)));                             \
        return *this;                                                                                                                      \
    }

    DEFINE_REQUESTMETHOD(use, "use")
    DEFINE_REQUESTMETHOD(all, "all")
    DEFINE_REQUESTMETHOD(get, "GET")
    DEFINE_REQUESTMETHOD(put, "PUT")
    DEFINE_REQUESTMETHOD(post, "POST")
    DEFINE_REQUESTMETHOD(del, "DELETE")
    DEFINE_REQUESTMETHOD(connect, "CONNECT")
    DEFINE_REQUESTMETHOD(options, "OPTIONS")
    DEFINE_REQUESTMETHOD(trace, "TRACE")
    DEFINE_REQUESTMETHOD(patch, "PATCH")
    DEFINE_REQUESTMETHOD(head, "HEAD")

} // namespace express
