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



#define DREQUESTMETHOD(METHOD)                                                                                                             \
    Router& METHOD(const std::string& path, const std::function<void(const Request& req, const Response& res)>& dispatcher);               \
    Router& METHOD(const std::string& path, Router& router);                                                                               \
    Router& METHOD(const std::string& path,                                                                                                \
                   const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher);


class MountPoint {
private:
    MountPoint(const std::string& method, const std::string& path)
        : method(method)
        , path(path) {
    }

    std::string method;
    std::string path;

    friend class Route;
    friend class Router;
    friend class DispatcherRoute;
    friend class MiddlewareRoute;
    friend class RouterRoute;
};


class RouterRoute;

class Router {
public:
    Router();

    DREQUESTMETHOD(use);
    DREQUESTMETHOD(all);
    DREQUESTMETHOD(get);
    DREQUESTMETHOD(put);
    DREQUESTMETHOD(post);
    DREQUESTMETHOD(del);
    DREQUESTMETHOD(connect);
    DREQUESTMETHOD(options);
    DREQUESTMETHOD(trace);
    DREQUESTMETHOD(patch);
    DREQUESTMETHOD(head);

    virtual bool dispatch(const Request& req, const Response& res) const;

protected:
    MountPoint mountPoint;
    std::shared_ptr<RouterRoute> routerRoute; // it can be shared by multiple routers
};

#endif // NEWROUTER_H
