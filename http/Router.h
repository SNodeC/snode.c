#ifndef NEWROUTER_H
#define NEWROUTER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "Response.h"

#define DREQUESTMETHOD(METHOD)                                                                                                             \
    Router& METHOD(const std::string& path, const std::function<void(Request & req, Response & res)>& dispatcher);                         \
    Router& METHOD(const std::function<void(Request & req, Response & res)>& dispatcher);                                                  \
    Router& METHOD(const std::string& path, const Router& router);                                                                         \
    Router& METHOD(const Router& router);                                                                                                  \
    Router& METHOD(const std::string& path,                                                                                                \
                   const std::function<void(Request & req, Response & res, const std::function<void(void)>& next)>& dispatcher);           \
    Router& METHOD(const std::function<void(Request & req, Response & res, const std::function<void(void)>& next)>& dispatcher);

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
    friend class ApplicationDispatcher;
    friend class MiddlewareDispatcher;
    friend class RouterDispatcher;
};

class RouterDispatcher;

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

    void dispatch(Request& req, Response& res) const;

    const std::shared_ptr<RouterDispatcher>& getRoute() const {
        return routerDispatcher;
    }

protected:
    static const MountPoint mountPoint;
    std::shared_ptr<RouterDispatcher> routerDispatcher; // it can be shared by multiple routers
};

#endif // NEWROUTER_H
