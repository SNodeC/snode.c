#ifndef ROUTER_H
#define ROUTER_H

#include <functional>
#include <list>
#include <map>
#include <string>

class Request;
class Response;
class Router;

class Route {
public:
    Route(const Router* parent, const std::string& path) : parent(parent), path(path) {}
    virtual ~Route() {}
    
    virtual bool dispatch(const std::string& method, const std::string& mpath, const Request& req, const Response& res) const = 0;
    
    
protected:
    const Router* parent;
    const std::string path;
    
friend class Router;
};


class RouterRoute : public Route {
public:
    RouterRoute(const Router* parent, std::string path, Router& router) : Route(parent, path), router(router) {}
    
    virtual bool dispatch(const std::string& method, const std::string& mpath, const Request& req, const Response& res) const;
    
private:
    const Router& router;
};


class DispatcherRoute : public Route {
public:
    DispatcherRoute(const Router* parent, const std::string& path, const std::function<void (const Request& req, const Response& res)>& dispatcher): Route(parent, path), dispatcher(dispatcher) {}
    
    virtual bool dispatch(const std::string& method, const std::string& mpath, const Request& req, const Response& res) const;
    
private:
    const std::function<void (const Request& req, const Response& res)> dispatcher;
};


class MiddlewareRoute : public Route {
public:
    MiddlewareRoute(const Router* parent, const std::string& path, const std::function<void (const Request& req, const Response& res, const std::function<void (void)>& next)>& dispatcher): Route (parent, path), dispatcher(dispatcher) {}
    
    virtual bool dispatch(const std::string& method, const std::string& mpath, const Request& req, const Response& res) const;
    
private:
    const std::function<void (const Request& req, const Response& res, std::function<void (void)>)> dispatcher;
};


#define REQUESTMETHOD(METHOD) \
void METHOD(const std::string& path, const std::function<void (const Request& req, const Response& res)>& dispatcher) { \
    routes[#METHOD].push_back(new DispatcherRoute(this, path, dispatcher));\
};\
\
void METHOD(const std::string& path, Router& router) { \
    routes[#METHOD].push_back(new RouterRoute(this, path, router));\
};\
\
void METHOD(const std::string& path, const std::function<void (const Request& req, const Response& res, const std::function<void (void)>& next)>& dispatcher) { \
    mroutes[#METHOD].push_back(new MiddlewareRoute(this, path, dispatcher)); \
};


class Router : public Route
{
public:
    Router() : Route(0, "") {}
    ~Router();
    
    REQUESTMETHOD(use);
    REQUESTMETHOD(get);
    REQUESTMETHOD(put);
    REQUESTMETHOD(post);
    REQUESTMETHOD(del); // for delete
    REQUESTMETHOD(connect);
    REQUESTMETHOD(options);
    REQUESTMETHOD(trace);
    REQUESTMETHOD(patch);
    REQUESTMETHOD(head);

    virtual bool dispatch(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const;
    bool dispatch(const std::map<const std::string, std::list<const Route*>>& routes, const std::string& method, const std::string& mpath, const Request& request, const Response& response) const;
    
protected:
    std::map<const std::string, std::list<const Route*>> mroutes;
    std::map<const std::string, std::list<const Route*>> routes;
};


#endif // ROUTER_H
