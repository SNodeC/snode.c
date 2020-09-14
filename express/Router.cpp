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

#include <filesystem>
#include <list>
#include <regex>
#include <vector>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Router.h"

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

    protected:
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

        // TODO: Fix regex-match
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

        // TODO: Fix regex-match
        if ((req.path.rfind(cpath, 0) == 0 && mountPoint.method == "use") ||
            ((cpath == req.path || checkForUrlMatch(cpath, req.url)) && (req.method == mountPoint.method || mountPoint.method == "all"))) {
            next = false;

            if (hasResult(cpath)) {
                setParams(cpath, req);
            }

            dispatcher(req, res, [&next]() -> void {
                next = true;
            });
        }

        return next;
    }

    bool ApplicationDispatcher::dispatch(const MountPoint& mountPoint, const std::string& parentPath, Request& req, Response& res) const {
        bool next = true;
        std::string cpath = path_concat(parentPath, mountPoint.path);

        // TODO: Fix regex-match
        if ((req.path.rfind(cpath, 0) == 0 && mountPoint.method == "use") ||
            ((cpath == req.path || checkForUrlMatch(cpath, req.url)) && (req.method == mountPoint.method || mountPoint.method == "all"))) {
            next = false;

            if (hasResult(cpath)) {
                setParams(cpath, req);
            }

            dispatcher(req, res);
        }

        return next;
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

    void Router::dispatch(const http::Request& req, const http::Response& res) {
        Request* expressReq = reqestMap[&req] = new Request(req);
        Response* expressRes = responseMap[&res] = new Response(res);

        static_cast<void>(routerDispatcher->dispatch(MountPoint("use", "/"), "/", *expressReq, *expressRes));
    }

    void Router::completed(const http::Request& req, const http::Response& res) {
        delete reqestMap[&req];
        reqestMap.erase(&req);

        delete responseMap[&res];
        responseMap.erase(&res);
    }

    DEFINE_REQUESTMETHOD(use, "use");
    DEFINE_REQUESTMETHOD(all, "all");
    DEFINE_REQUESTMETHOD(get, "GET");
    DEFINE_REQUESTMETHOD(put, "PUT");
    DEFINE_REQUESTMETHOD(post, "POST");
    DEFINE_REQUESTMETHOD(del, "DELETE");
    DEFINE_REQUESTMETHOD(connect, "CONNECT");
    DEFINE_REQUESTMETHOD(options, "OPTIONS");
    DEFINE_REQUESTMETHOD(trace, "TRACE");
    DEFINE_REQUESTMETHOD(patch, "PATCH");
    DEFINE_REQUESTMETHOD(head, "HEAD");

} // namespace express
