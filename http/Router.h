#ifndef NEWROUTER_H
#define NEWROUTER_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <list>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "Response.h"


class Router;

class MountPoint {
public:
    MountPoint(const std::string& method, const std::string& path)
        : method(method)
        , path(path) {
    }

    std::string method;
    std::string path;
};


class Dispatcher {
public:
    Dispatcher() {
    }
    virtual ~Dispatcher() = default;

    virtual bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, const Request& req,
                          const Response& res) const = 0;
};


class Route {
public:
    Route(Router* parent, const std::string& method, const std::string& path, const std::shared_ptr<Dispatcher>& route)
        : parent(parent)
        , mountPoint(method, path)
        , route(route) {
    }

    bool dispatch(const std::string& parentPath, const Request& req, const Response& res) const;

protected:
    Router* parent;
    MountPoint mountPoint;
    std::shared_ptr<Dispatcher> route;
};


class RouterRoute : public Dispatcher {
public:
    virtual bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, const Request& req,
                          const Response& res) const;

protected:
    std::list<Route> routes;

    friend class Router;
};


class MiddlewareRoute : public Dispatcher {
public:
    MiddlewareRoute(const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher)
        : dispatcher(dispatcher) {
    }

    virtual bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, const Request& req,
                          const Response& res) const;

protected:
    const std::function<void(const Request& req, const Response& res, std::function<void(void)>)> dispatcher;
};


class DispatcherRoute : public Dispatcher {
public:
    DispatcherRoute(const std::function<void(const Request& req, const Response& res)>& dispatcher)
        : dispatcher(dispatcher) {
    }

    virtual bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, const Request& req,
                          const Response& res) const;

protected:
    const std::function<void(const Request& req, const Response& res)> dispatcher;
};


#define REQUESTMETHOD(METHOD, HTTP_METHOD)                                                                                                 \
    Router& METHOD(const std::string& path, const std::function<void(const Request& req, const Response& res)>& dispatcher) {              \
        routerRoute->routes.push_back(Route(this, HTTP_METHOD, path, std::make_shared<DispatcherRoute>(dispatcher)));                      \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& METHOD(const std::string& path, Router& router) {                                                                              \
        routerRoute->routes.push_back(Route(this, HTTP_METHOD, path, router.routerRoute));                                                 \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& METHOD(                                                                                                                        \
        const std::string& path,                                                                                                           \
        const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher) {           \
        routerRoute->routes.push_back(Route(this, HTTP_METHOD, path, std::make_shared<MiddlewareRoute>(dispatcher)));                      \
        return *this;                                                                                                                      \
    };


class Router {
public:
    Router()
        : mountPoint("use", "/")
        , routerRoute(new RouterRoute()) {
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

    virtual bool dispatch(const Request& req, const Response& res) const;

protected:
    MountPoint mountPoint;
    std::shared_ptr<RouterRoute> routerRoute; // it can be shared by multiple routers
};

#endif // NEWROUTER_H
