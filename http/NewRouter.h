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
    Request(const std::string& path) : path(path) {}
    
    std::string path;
};

class Response {
};


namespace NEW {

class Router;

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

class Route {
public:
    Route(Router* parent, const std::string& method, const std::string& path)
        : parent(parent)
        , method(method)
        , path(path) {
    }
    virtual ~Route() = default;

    virtual bool dispatch(const std::string& method, const std::string& parentPath, const Request& req, const Response& res) const = 0;

//protected:
    Router* parent;
    std::string method;
    std::string path;

private:
};


class RouterRoute : public Route {
public:
    RouterRoute(Router* parent, const std::string& method, const std::string& path)
    : Route(parent, method, path) {
    }
    
    virtual bool dispatch(const std::string& method, const std::string& parentPath, const Request& req, const Response& res) const {
        bool next = true;
        std::string cpath = path_concat(parentPath, path);
        
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        std::cout << "    Request Method: " << method << std::endl;
        std::cout << "    Request Path: " << req.path << std::endl;
        std::cout << "    Registered For: " << this->method << std::endl;
        std::cout << "    Parent Path: " << parentPath << std::endl;
        std::cout << "    Registered Path: " << this->path << std::endl;
        std::cout << "    Combined Path: " << cpath << std::endl;
        std::cout << "    Path Match: " << req.path.rfind(cpath, 0) << std::endl;
        
        if (req.path.rfind(cpath, 0) == 0 && (this->method == method || this->method == "use")) {
            std::cout << "    -----" << std::endl;
            std::cout << "    MATCH" << std::endl;
            std::cout << "    -----" << std::endl;
            /*
            req.url = req.originalUrl.substr(cpath.length());
            if (req.url.front() != '/') {
                req.url.insert(0, "/");
            }
            */
            std::list<std::shared_ptr<Route>>::const_iterator itb = routes.begin();
            std::list<std::shared_ptr<Route>>::const_iterator ite = routes.end();

            while (itb != ite && next) {
                next = (*itb)->dispatch(method, cpath, req, res);
                ++itb;
            }
        }
        return next;
    }
    
//protected:
    std::list<std::shared_ptr<Route>> routes;
};


class MiddlewareRoute : public Route {
public:
    MiddlewareRoute(Router* parent, const std::string& method, const std::string& path,
                    const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher)
    : Route(parent, method, path), dispatcher(dispatcher){
    }
    
    virtual bool dispatch(const std::string& method, const std::string& parentPath, const Request& req, const Response& res) const {
        bool next = true;
        std::string cpath = path_concat(parentPath, path);

        std::cout << __PRETTY_FUNCTION__ << std::endl;
        std::cout << "    Request Method: " << method << std::endl;
        std::cout << "    Request Path: " << req.path << std::endl;
        std::cout << "    Registered For: " << this->method << std::endl;
        std::cout << "    Parent Path: " << parentPath << std::endl;
        std::cout << "    Registered Path: " << this->path << std::endl;
        std::cout << "    Combined Path: " << cpath << std::endl;
        std::cout << "    Path Match: " << req.path.rfind(cpath, 0) << std::endl;
        
        if ((req.path.rfind(cpath, 0) == 0 && this->method == "use") ||
            (cpath == req.path && (method == this->method || this->method == "all"))) {
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
    DispatcherRoute(Router* parent, const std::string& method, const std::string& path,
                    const std::function<void(const Request& req, const Response& res)>& dispatcher)
    : Route(parent, method, path), dispatcher(dispatcher) {
    }
    
    virtual bool dispatch(const std::string& method, const std::string& parentPath, const Request& req, const Response& res) const {
        bool next = true;
        std::string cpath = path_concat(parentPath, path);
        
        std::cout << __PRETTY_FUNCTION__ << std::endl;
        std::cout << "    Request Method: " << method << std::endl;
        std::cout << "    Request Path: " << req.path << std::endl;
        std::cout << "    Registered For: " << this->method << std::endl;
        std::cout << "    Parent Path: " << parentPath << std::endl;
        std::cout << "    Registered Path: " << this->path << std::endl;
        std::cout << "    Combined Path: " << cpath << std::endl;
        std::cout << "    Path Match: " << (cpath == req.path) << std::endl;
        

        if (cpath == req.path && (method == this->method || this->method == "all")) {
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
        routerRoute->routes.push_back(std::make_shared<DispatcherRoute>(this, HTTP_METHOD, path, dispatcher));                             \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& METHOD(const std::string& path, Router& router) { \
        router.routerRoute->path = path; \
        router.routerRoute->method = HTTP_METHOD; \
        router.routerRoute->parent = this; \
        routerRoute->routes.push_back(router.routerRoute);                                                                                 \
        return *this;                                                                                                                      \
    };                                                                                                                                     \
                                                                                                                                           \
    Router& METHOD(                                                                                                                        \
        const std::string& path,                                                                                                           \
        const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher) {           \
        routerRoute->routes.push_back(std::make_shared<MiddlewareRoute>(this, HTTP_METHOD, path, dispatcher));                             \
        return *this;                                                                                                                      \
    };

    
class Router {
public:
    Router() : routerRoute(new RouterRoute(0, "use", "/")) {}
    
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
        return routerRoute->dispatch(method, "/", req, res);
    }
    
protected:
    std::shared_ptr<RouterRoute> routerRoute; // it can be shared by multiple routers
};

} // end namespace NEW

#endif // NEWROUTER_H
