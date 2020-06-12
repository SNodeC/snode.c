#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Router.h"


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


class Dispatcher {
public:
    Dispatcher() = default;
    Dispatcher(const Dispatcher&) = delete;

    Dispatcher& operator=(const Dispatcher&) = delete;

    virtual ~Dispatcher() = default;

    virtual bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, const Request& req, const Response& res) const = 0;
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
    RouterRoute() = default;

    RouterRoute& operator=(const RouterRoute&) = delete;

    RouterRoute(const RouterRoute&) = delete;

    virtual bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, const Request& req, const Response& res) const;

protected:
    std::list<Route> routes;

    friend class Router;
};


class MiddlewareRoute : public Dispatcher {
public:
    MiddlewareRoute(const MiddlewareRoute&) = delete;

    MiddlewareRoute& operator=(MiddlewareRoute&) = delete;

    MiddlewareRoute(const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher)
        : dispatcher(dispatcher) {
    }

    virtual bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, const Request& req, const Response& res) const;

protected:
    const std::function<void(const Request& req, const Response& res, std::function<void(void)>)> dispatcher;
};


class DispatcherRoute : public Dispatcher {
public:
    DispatcherRoute(const DispatcherRoute&) = delete;

    DispatcherRoute& operator=(DispatcherRoute&) = delete;

    DispatcherRoute(const std::function<void(const Request& req, const Response& res)>& dispatcher)
        : dispatcher(dispatcher) {
    }

    virtual bool dispatch(const MountPoint& mountPoint, const std::string& parentPath, const Request& req, const Response& res) const;

protected:
    const std::function<void(const Request& req, const Response& res)> dispatcher;
};


bool Route::dispatch(const std::string& parentPath, const Request& req, const Response& res) const {
    return route->dispatch(mountPoint, parentPath, req, res);
}


bool RouterRoute::dispatch(const MountPoint& mountPoint, const std::string& parentPath, const Request& req, const Response& res) const {
    bool next = true;
    std::string cpath = path_concat(parentPath, mountPoint.path);

    if (req.path.rfind(cpath, 0) == 0 && (mountPoint.method == req.method() || mountPoint.method == "use")) {
        req.url = req.originalUrl.substr(cpath.length());

        if (req.url.front() != '/') {
            req.url.insert(0, "/");
        }

        std::list<Route>::const_iterator route = routes.begin();
        std::list<Route>::const_iterator end = routes.end();

        while (route != end && next) { // todo: to be exchanged by an stl-algorithm
            next = route->dispatch(cpath, req, res);
            ++route;
        }
    }
    return next;
}


bool MiddlewareRoute::dispatch(const MountPoint& mountPoint, const std::string& parentPath, const Request& req, const Response& res) const {
    bool next = true;
    std::string cpath = path_concat(parentPath, mountPoint.path);

    if ((req.path.rfind(cpath, 0) == 0 && mountPoint.method == "use") ||
        (cpath == req.path && (req.method() == mountPoint.method || mountPoint.method == "all"))) {
        req.url = req.originalUrl.substr(cpath.length());

        if (req.url.front() != '/') {
            req.url.insert(0, "/");
        }

        next = false;
        this->dispatcher(req, res, [&next](void) -> void {
            next = true;
        });
    }
    return next;
}


bool DispatcherRoute::dispatch(const MountPoint& mountPoint, const std::string& parentPath, const Request& req, const Response& res) const {
    bool next = true;
    std::string cpath = path_concat(parentPath, mountPoint.path);

    if (cpath == req.path && (req.method() == mountPoint.method || mountPoint.method == "all")) {
        req.url = req.originalUrl.substr(cpath.length());

        if (req.url.front() != '/') {
            req.url.insert(0, "/");
        }

        this->dispatcher(req, res);

        next = false;
    }

    return next;
}


Router::Router()
    : mountPoint("use", "/")
    , routerRoute(new RouterRoute()) {
}


bool Router::dispatch(const Request& req, const Response& res) const {
    return routerRoute->dispatch(mountPoint, "/", req, res);
}


#define REQUESTMETHOD(METHOD, HTTP_METHOD)                                                                                                 \
    Router& Router::METHOD(const std::string& path, const std::function<void(const Request& req, const Response& res)>& dispatcher) {      \
        routerRoute->routes.push_back(Route(this, HTTP_METHOD, path, std::make_shared<DispatcherRoute>(dispatcher)));                      \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& Router::METHOD(const std::function<void(const Request& req, const Response& res)>& dispatcher) {                               \
        routerRoute->routes.push_back(Route(this, HTTP_METHOD, "", std::make_shared<DispatcherRoute>(dispatcher)));                        \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& Router::METHOD(const std::string& path, const Router& router) {                                                                \
        routerRoute->routes.push_back(Route(this, HTTP_METHOD, path, router.routerRoute));                                                 \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& Router::METHOD(const Router& router) {                                                                                         \
        routerRoute->routes.push_back(Route(this, HTTP_METHOD, "", router.routerRoute));                                                   \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& Router::METHOD(                                                                                                                \
        const std::string& path,                                                                                                           \
        const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher) {           \
        routerRoute->routes.push_back(Route(this, HTTP_METHOD, path, std::make_shared<MiddlewareRoute>(dispatcher)));                      \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& Router::METHOD(                                                                                                                \
        const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher) {           \
        routerRoute->routes.push_back(Route(this, HTTP_METHOD, "", std::make_shared<MiddlewareRoute>(dispatcher)));                        \
        return *this;                                                                                                                      \
    };


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
