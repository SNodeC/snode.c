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
    Router& METHOD(const std::function<void(const Request& req, const Response& res)>& dispatcher);                                        \
    Router& METHOD(const std::string& path, const Router& router);                                                                         \
    Router& METHOD(const Router& router);                                                                                                  \
    Router& METHOD(const std::string& path,                                                                                                \
                   const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher); \
    Router& METHOD(const std::function<void(const Request& req, const Response& res, const std::function<void(void)>& next)>& dispatcher);


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

namespace tls {
    class WebApp;
} // namespace tls

namespace legacy {
    class WebApp;
} // namespace legacy

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

    void dispatch(const Request& req, const Response& res) const;

protected:
    const std::shared_ptr<RouterRoute>& getRoute() const {
        return routerRoute;
    }

    void setRoute(const std::shared_ptr<RouterRoute>& route) {
        routerRoute = route;
    }

    const MountPoint& getMountPoint() const {
        return mountPoint;
    }

    void setMountPoint(const MountPoint& mountPoint) {
        this->mountPoint = mountPoint;
    }

protected:
    MountPoint mountPoint;
    std::shared_ptr<RouterRoute> routerRoute; // it can be shared by multiple routers

    friend class tls::WebApp;
    friend class legacy::WebApp;
};

#endif // NEWROUTER_H
