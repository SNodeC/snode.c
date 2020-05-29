#ifndef NEWROUTER_H
#define NEWROUTER_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

//#include "Request.h"
//#include "Response.h"

class Request {
public:
    Request(const std::string& path)
        : path(path) {
    }

    std::string path;
};

class Response {};


namespace NEW {

    static const std::string path_concat(const std::string& first, const std::string& second) {
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

    class Router;
    class RouteProps;
    
    class Route {
    public:
        Route() {}
        virtual ~Route() = default;

        virtual bool dispatch(const RouteProps& props, const std::string& method, const std::string& parentPath, const Request& req,
                              const Response& res) const = 0;
    };


    class RouteProps {
    public:
        RouteProps(Router* parent, const std::string& method, const std::string& path, const std::shared_ptr<Route>& route)
            : parent(parent)
            , method(method)
            , path(path)
            , route(route) {
        }

        Router* parent;
        std::string method;
        std::string path;
        std::shared_ptr<Route> route;
    };


    class RouterRoute : public Route {
    public:
        RouterRoute() {}

        virtual bool dispatch(const RouteProps& props, const std::string& method, const std::string& parentPath, const Request& req,
                              const Response& res) const {
            bool next = true;
            std::string cpath = path_concat(parentPath, props.path);

            std::cout << __PRETTY_FUNCTION__ << std::endl;
            std::cout << "    Request Method: " << method << std::endl;
            std::cout << "    Request Path: " << req.path << std::endl;
            std::cout << "    Registered For: " << props.method << std::endl;
            std::cout << "    Parent Path: " << parentPath << std::endl;
            std::cout << "    Registered Path: " << props.path << std::endl;
            std::cout << "    Combined Path: " << cpath << std::endl;
            std::cout << "    Path Match: " << req.path.rfind(cpath, 0) << std::endl;

            if (req.path.rfind(cpath, 0) == 0 && (props.method == method || props.method == "use")) {
                std::cout << "    -----" << std::endl;
                std::cout << "    MATCH" << std::endl;
                std::cout << "    -----" << std::endl;
                /*
                req.url = req.originalUrl.substr(cpath.length());
                if (req.url.front() != '/') {
                    req.url.insert(0, "/");
                }
                */
                std::list<std::pair<RouteProps, std::shared_ptr<Route>>>::const_iterator itb = routes.begin();
                std::list<std::pair<RouteProps, std::shared_ptr<Route>>>::const_iterator ite = routes.end();

                while (itb != ite && next) {
                    next = (*itb).first.route->dispatch((*itb).first, method, cpath, req, res);
                    ++itb;
                }
            }
            return next;
        }

        // protected:
        std::list<std::pair<RouteProps, std::shared_ptr<Route>>> routes;
    };


    class MiddlewareRoute : public Route {
    public:
        MiddlewareRoute(const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher)
            : dispatcher(dispatcher) {
        }

        virtual bool dispatch(const RouteProps& props, const std::string& method, const std::string& parentPath, const Request& req,
                              const Response& res) const {
            bool next = true;
            std::string cpath = path_concat(parentPath, props.path);

            std::cout << __PRETTY_FUNCTION__ << std::endl;
            std::cout << "    Request Method: " << method << std::endl;
            std::cout << "    Request Path: " << req.path << std::endl;
            std::cout << "    Registered For: " << props.method << std::endl;
            std::cout << "    Parent Path: " << parentPath << std::endl;
            std::cout << "    Registered Path: " << props.path << std::endl;
            std::cout << "    Combined Path: " << cpath << std::endl;
            std::cout << "    Path Match: " << req.path.rfind(cpath, 0) << std::endl;

            if ((req.path.rfind(cpath, 0) == 0 && props.method == "use") ||
                (cpath == req.path && (method == props.method || props.method == "all"))) {
                std::cout << "    -----" << std::endl;
                std::cout << "    MATCH" << std::endl;
                std::cout << "    -----" << std::endl;
                /*
                next = false;
                req.url = req.originalUrl.substr(cpath.length());
                if (req.url.front() != '/') {
                    req.url.insert(0, "/");
                }
                */
                std::cout << ">>>>>>>" << std::endl;
                this->dispatcher(req, res, [&next](void) -> void {
                    next = true;
                });
                std::cout << "<<<<<<<" << std::endl;
            }
            return next;
        }

    protected:
        const std::function<void(const Request& req, const Response& res, std::function<void(void)>)> dispatcher;
    };


    class DispatcherRoute : public Route {
    public:
        DispatcherRoute(const std::function<void(const Request& req, const Response& res)>& dispatcher)
            : dispatcher(dispatcher) {
        }

        virtual bool dispatch(const RouteProps& props, const std::string& method, const std::string& parentPath, const Request& req,
                              const Response& res) const {
            bool next = true;
            std::string cpath = path_concat(parentPath, props.path);

            std::cout << __PRETTY_FUNCTION__ << std::endl;
            std::cout << "    Request Method: " << method << std::endl;
            std::cout << "    Request Path: " << req.path << std::endl;
            std::cout << "    Registered For: " << props.method << std::endl;
            std::cout << "    Parent Path: " << parentPath << std::endl;
            std::cout << "    Registered Path: " << props.path << std::endl;
            std::cout << "    Combined Path: " << cpath << std::endl;
            std::cout << "    Path Match: " << (cpath == req.path) << std::endl;


            if (cpath == req.path && (method == props.method || props.method == "all")) {
                std::cout << "    -----" << std::endl;
                std::cout << "    MATCH" << std::endl;
                std::cout << "    -----" << std::endl;
                /*
                    req.url = req.originalUrl.substr(cpath.length());
                    if (req.url.front() != '/') {
                        req.url.insert(0, "/");
                    }
                */
                std::cout << ">>>>>>>" << std::endl;
                this->dispatcher(req, res);
                std::cout << "<<<<<<<" << std::endl;
                next = false;
            }

            return next;
        }

    protected:
        const std::function<void(const Request& req, const Response& res)> dispatcher;
    };


#define REQUESTMETHOD(METHOD, HTTP_METHOD)                                                                                                 \
    Router& METHOD(const std::string& path, const std::function<void(const Request& req, const Response& res)>& dispatcher) {              \
        std::shared_ptr<DispatcherRoute> dr = std::make_shared<DispatcherRoute>(dispatcher);\
        routerRoute->routes.push_back(std::pair<RouteProps, std::shared_ptr<Route>>(                                                       \
            RouteProps(this, HTTP_METHOD, path, dr), dr));                                          \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& METHOD(const std::string& path, Router& router) {                                                                              \
        routerRoute->routes.push_back(                                                                                                     \
            std::pair<RouteProps, std::shared_ptr<Route>>(RouteProps(this, HTTP_METHOD, path, router.routerRoute), router.routerRoute));                       \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& METHOD(                                                                                                                        \
        const std::string& path,                                                                                                           \
        const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher) {           \
        std::shared_ptr<MiddlewareRoute> mr = std::make_shared<MiddlewareRoute>(dispatcher); \
        routerRoute->routes.push_back(std::pair<RouteProps, std::shared_ptr<Route>>(                                                       \
            RouteProps(this, HTTP_METHOD, path, mr), mr));                                          \
        return *this;                                                                                                                      \
    };


    class Router {
    public:
        Router()
            : routerRoute(new RouterRoute()) {
        }

        REQUESTMETHOD(use, "use");
        REQUESTMETHOD(all, "all");
        REQUESTMETHOD(get, "get");
        REQUESTMETHOD(put, "put");
        REQUESTMETHOD(post, "post");
        REQUESTMETHOD(del, "delete");
        REQUESTMETHOD(connect, "connect");
        REQUESTMETHOD(options, "options");
        REQUESTMETHOD(trace, "trace");
        REQUESTMETHOD(patch, "patch");
        REQUESTMETHOD(head, "head");

        virtual bool dispatch(const std::string& method, const Request& req, const Response& res) const {
            RouteProps routeProps(0, "use", "/", routerRoute);
            return routerRoute->dispatch(routeProps, method, "/", req, res);
        }

    protected:
        std::shared_ptr<RouterRoute> routerRoute; // it can be shared by multiple routers
    };

} // end namespace NEW

#endif // NEWROUTER_H
