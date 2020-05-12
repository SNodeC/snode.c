#ifndef ROUTER_H
#define ROUTER_H

#include <functional>
#include <map>
#include <string>

class Request;
class Response;

class Route {
public:
    Route(std::string path, const std::function<void (const Request& req, const Response& res)>& processor) : path(path), processor(processor) {}

    std::string path;
    const std::function<void (const Request& req, const Response& res)> processor;
};
    

#define METHOD(M) \
virtual void M(const std::string& path, const std::function<void (const Request& req, const Response& res)>& processor) { \
    routes[ #M ].insert({path, Route(path, processor)}); \
}

/*
//    Route r(path, processor); \
routes[M].insert(Route(path, processor)); \
//    M ## Routes.insert({path, r}); \
}
*/


class Router
{
public:
    METHOD(all);
    METHOD(get);
    METHOD(put);
    METHOD(post);
    METHOD(del); // for delete
    METHOD(connect);
    METHOD(options);
    METHOD(trace);
    METHOD(patch);
    METHOD(head);

protected:
    std::map<std::string, std::map<std::string, Route>> routes;
};

#endif // ROUTER_H
