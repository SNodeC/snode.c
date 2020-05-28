#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "Response.h"
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


RouterRoute::RouterRoute(const Router* parent, const std::string& method, std::string path, const Router* router)
    : Route(parent, method, path)
    , router(router) {
}


bool RouterRoute::dispatch(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;

    std::string cpath = path_concat(mpath, path);

    if (request.path.rfind(cpath, 0) == 0 && method == this->method) {
        request.url = request.originalUrl.substr(cpath.length());
        if (request.url.front() != '/') {
            request.url.insert(0, "/");
        }
        next = router->dispatch(method, cpath, request, response);
    }

    return next;
}


const Route* RouterRoute::clone(const Router* parent) const {
    return new RouterRoute(parent, method, path, new Router(*router));
}


DispatcherRoute::DispatcherRoute(const Router* parent, const std::string& method, const std::string& path,
                                 const std::function<void(const Request& req, const Response& res)>& dispatcher)
    : Route(parent, method, path)
    , dispatcher(dispatcher) {
}


bool DispatcherRoute::dispatch(const std::string& method, const std::string& mpath, const Request& request,
                               const Response& response) const {
    bool next = true;

    std::string cpath = path_concat(mpath, path);

    if ((request.path.rfind(cpath, 0) == 0 && this->method == "use") ||
        (cpath == request.path && (method == this->method || this->method == "all"))) {
        request.url = request.originalUrl.substr(cpath.length());
        if (request.url.front() != '/') {
            request.url.insert(0, "/");
        }
        this->dispatcher(request, response);
        next = false;
    }

    return next;
}


const Route* DispatcherRoute::clone(const Router* parent) const {
    return new DispatcherRoute(parent, method, path, dispatcher);
}


MiddlewareRoute::MiddlewareRoute(
    const Router* parent, const std::string& method, const std::string& path,
    const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher)
    : Route(parent, method, path)
    , dispatcher(dispatcher) {
}


bool MiddlewareRoute::dispatch(const std::string& method, const std::string& mpath, const Request& request,
                               const Response& response) const {
    bool next = true;

    std::string cpath = path_concat(mpath, path);

    if ((request.path.rfind(cpath, 0) == 0 && this->method == "use") ||
        (cpath == request.path && (method == this->method || this->method == "all"))) {
        next = false;
        request.url = request.originalUrl.substr(cpath.length());
        if (request.url.front() != '/') {
            request.url.insert(0, "/");
        }
        this->dispatcher(request, response, [&next](void) -> void {
            next = true;
        });
    }

    return next;
}


const Route* MiddlewareRoute::clone(const Router* parent) const {
    return new MiddlewareRoute(parent, method, path, dispatcher);
}


Router::Router()
    : Route(0, "use", "") {
}


Router::Router(const Router& router)
    : Route(0, "use", "") {
    std::for_each(router.routes.begin(), router.routes.end(), [this](const Route* route) -> void {
        this->routes.push_back(route->clone(this));
    });
}


Router& Router::operator=(const Router& router) {
    this->clear();

    std::for_each(router.routes.begin(), router.routes.end(), [this](const Route* route) -> void {
        this->routes.push_back(route->clone(this));
    });

    return *this;
}


void Router::clear() {
    std::for_each(routes.begin(), routes.end(), [](const Route* route) -> void {
        delete route;
    });

    this->routes.clear();
}


Router::~Router() {
    this->clear();
}


bool Router::dispatch(const std::string& method, const std::string& path, const Request& request, const Response& response) const {
    bool next = true;

    std::for_each(routes.begin(), routes.end(), [&next, &method, &path, &request, &response](const Route* route) -> void {
        next = route->dispatch(method, path, request, response);
    });

    return next;
}
