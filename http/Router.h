#ifndef ROUTER_H
#define ROUTER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <string>

#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

class Request;
class Response;
class Router;

class Route {
public:
    Route(const Router* parent, const std::string& method, const std::string& path)
        : parent(parent)
        , method(method)
        , path(path) {
    }
    virtual ~Route() {
    }

    virtual bool dispatch(const std::string& method, const std::string& mpath, const Request& req, const Response& res) const = 0;

protected:
    const Router* parent;
    const std::string method;
    const std::string path;

private:
    virtual const Route* clone(const Router* parent) const = 0;
    
    Route(const Route& router) = delete;
    Route& operator=(const Route& route) = delete;

    friend class Router;
};


class RouterRoute : public Route {
public:
    RouterRoute(const Router* parent, const std::string& method, std::string path, const Router* router)
        : Route(parent, method, path)
        , router(router) {
    }

    virtual bool dispatch(const std::string& method, const std::string& mpath, const Request& req, const Response& res) const;

private:
    virtual const Route* clone(const Router* parent) const;
    const Router* router;
};


class DispatcherRoute : public Route {
public:
    DispatcherRoute(const Router* parent, const std::string& method, const std::string& path,
                    const std::function<void(const Request& req, const Response& res)>& dispatcher)
        : Route(parent, method, path)
        , dispatcher(dispatcher) {
    }

    virtual bool dispatch(const std::string& method, const std::string& mpath, const Request& req, const Response& res) const;

private:
    virtual const Route* clone(const Router* parent) const;

    const std::function<void(const Request& req, const Response& res)> dispatcher;
};


class MiddlewareRoute : public Route {
public:
    MiddlewareRoute(const Router* parent, const std::string& method, const std::string& path,
                    const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher)
        : Route(parent, method, path)
        , dispatcher(dispatcher) {
    }

    virtual bool dispatch(const std::string& method, const std::string& mpath, const Request& req, const Response& res) const;

private:
    virtual const Route* clone(const Router* parent) const;
    const std::function<void(const Request& req, const Response& res, std::function<void(void)>)> dispatcher;
};


#define REQUESTMETHOD(METHOD, HTTP_METHOD)                                                                                                 \
    Router& METHOD(const std::string& path, const std::function<void(const Request& req, const Response& res)>& dispatcher) {              \
        routes.push_back(new DispatcherRoute(this, HTTP_METHOD, path, dispatcher));                                                        \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& METHOD(const std::string& path, Router& router) {                                                                              \
        routes.push_back(new RouterRoute(this, HTTP_METHOD, path, &router));                                                               \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& METHOD(                                                                                                                        \
        const std::string& path,                                                                                                           \
        const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher) {           \
        routes.push_back(new MiddlewareRoute(this, HTTP_METHOD, path, dispatcher));                                                        \
        return *this;                                                                                                                      \
    };


class Router : public Route {
public:
    Router()
        : Route(0, "use", "") {
    }
    ~Router();

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

    virtual bool dispatch(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const;

    
    Router(const Router& sRouter) : Route(0, "use", "") {
        std::for_each(sRouter.routes.begin(), sRouter.routes.end(),
            [this] (const Route* route) -> void {
                this->routes.push_back(route->clone(this));
            }
        );
    }
    
    Router& operator=(const Router& sRouter) {
        this->routes.clear();  // todo destructor?
        
        std::for_each(sRouter.routes.begin(), sRouter.routes.end(),
            [this] (const Route* route) -> void {
                this->routes.push_back(route->clone(this));
            }
        );
        
        return *this;
    }
    
protected:
    std::list<const Route*> routes;
    
private:
    virtual const Route* clone(const Router* parent) const {
        return 0;
    }
};


#endif // ROUTER_H
