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
    
    virtual bool process(const std::string& method, const std::string& mpath, const Request& req, const Response& res) const = 0;
    
    
protected:
    const Router* parent;
    const std::string path;
    
friend class Router;
};


class RRoute : public Route {
public:
    RRoute(const Router* parent, std::string path, Router& router) : Route(parent, path), router(router) {}
    
    virtual bool process(const std::string& method, const std::string& mpath, const Request& req, const Response& res) const;
    
private:
    const Router& router;
};


class PRoute : public Route {
public:
    PRoute(const Router* parent, const std::string& path, const std::function<void (const Request& req, const Response& res)>& processor): Route(parent, path), processor(processor) {}
    
    virtual bool process(const std::string& method, const std::string& mpath, const Request& req, const Response& res) const;
    
private:
    const std::function<void (const Request& req, const Response& res)> processor;
};


class MRoute : public Route {
public:
    MRoute(const Router* parent, const std::string& path, const std::function<void (const Request& req, const Response& res, const std::function<void (void)>& next)>& processor): Route (parent, path), processor(processor) {}
    
    virtual bool process(const std::string& method, const std::string& mpath, const Request& req, const Response& res) const;
    
private:
    const std::function<void (const Request& req, const Response& res, std::function<void (void)>)> processor;
};


#define METHOD(M) \
void M(const std::string& path, const std::function<void (const Request& req, const Response& res)>& processor) { \
    routes[#M].push_back(new PRoute(this, path, processor));\
};\
void M(const std::string& path, Router& router) { \
    routes[#M].push_back(new RRoute(this, path, router));\
};\
void M(const std::string& path, const std::function<void (const Request& req, const Response& res, const std::function<void (void)>& next)>& processor) { \
    mroutes[#M].push_back(new MRoute(this, path, processor)); \
};


class Router : public Route
{
public:
    Router() : Route(0, "") {}
    METHOD(use);
    METHOD(get);
    METHOD(put);
    METHOD(post);
    METHOD(del); // for delete
    METHOD(connect);
    METHOD(options);
    METHOD(trace);
    METHOD(patch);
    METHOD(head);

    virtual bool process(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const;
    bool process(const std::map<const std::string, std::list<const Route*>>& routes, const std::string& method, const std::string& mpath, const Request& request, const Response& response) const;
    
protected:
    std::map<const std::string, std::list<const Route*>> mroutes;
    std::map<const std::string, std::list<const Route*>> routes;
};


#endif // ROUTER_H
