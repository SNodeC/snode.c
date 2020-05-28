#ifndef ROUTER_H
#define ROUTER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <list>
#include <map>
#include <string>

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

    friend class Router;
};


class RouterRoute : public Route {
public:
    RouterRoute(const Router* parent, const std::string& method, std::string path, Router& router)
        : Route(parent, method, path)
        , router(router) {
    }

    virtual bool dispatch(const std::string& method, const std::string& mpath, const Request& req, const Response& res) const;

private:
    const Router& router;
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
    const std::function<void(const Request& req, const Response& res, std::function<void(void)>)> dispatcher;
};


#define REQUESTMETHOD(METHOD, HTTP_METHOD)                                                                                                 \
    void METHOD(const std::string& path, const std::function<void(const Request& req, const Response& res)>& dispatcher) {                 \
        routes.push_back(new DispatcherRoute(this, HTTP_METHOD, path, dispatcher));                                                        \
    };                                                                                                                                     \
                                                                                                                                           \
    void METHOD(const std::string& path, Router& router) {                                                                                 \
        routes.push_back(new RouterRoute(this, HTTP_METHOD, path, router));                                                                \
    };                                                                                                                                     \
                                                                                                                                           \
    void METHOD(const std::string& path,                                                                                                   \
                const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher) {   \
        routes.push_back(new MiddlewareRoute(this, HTTP_METHOD, path, dispatcher));                                                        \
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

    bool dispatch(const std::list<const Route*>& nroute, const std::string& method, const std::string& mpath, const Request& request,
                  const Response& response) const;

    virtual bool dispatch(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const;


protected:
    std::list<const Route*> routes;
};


#endif // ROUTER_H
