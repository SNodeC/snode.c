#ifndef ROUTER_H
#define ROUTER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <functional>
#include <iostream>
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

private:
    virtual const Route* clone(const Router* parent) const {
        return 0;
    }

    Route(const Route& route) = delete;
    Route& operator=(const Route& route) = delete;

    friend class Router;
};


class RouterRoute : public Route {
public:
    RouterRoute(const Router* parent, const std::string& method, std::string path, const Router* router);

    virtual bool dispatch(const std::string& method, const std::string& mpath, const Request& req, const Response& res) const;

private:
    virtual const Route* clone(const Router* parent) const;
    const Router* router;
};


class DispatcherRoute : public Route {
public:
    DispatcherRoute(const Router* parent, const std::string& method, const std::string& path,
                    const std::function<void(const Request& req, const Response& res)>& dispatcher);

    virtual bool dispatch(const std::string& method, const std::string& mpath, const Request& req, const Response& res) const;

private:
    virtual const Route* clone(const Router* parent) const;

    const std::function<void(const Request& req, const Response& res)> dispatcher;
};


class MiddlewareRoute : public Route {
public:
    MiddlewareRoute(const Router* parent, const std::string& method, const std::string& path,
                    const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher);

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
    Router();
    Router(const Router& router);
    Router& operator=(const Router& router);

    void clear();
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


protected:
    std::list<const Route*> routes;
};


#endif // ROUTER_H
