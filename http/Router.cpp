#include "Router.h"


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


bool Route::dispatch(const std::string& parentPath, const Request& req, const Response& res) const {
    return route->dispatch(mountPoint, parentPath, req, res);
}


bool RouterRoute::dispatch(const MountPoint& mountPoint, const std::string& parentPath, const Request& req,
                           const Response& res) const {
    bool next = true;
    std::string cpath = path_concat(parentPath, mountPoint.path);
    
    if (req.path.rfind(cpath, 0) == 0 && (mountPoint.method == req.method() || mountPoint.method == "use")) {
        req.url = req.originalUrl.substr(cpath.length());
        
        if (req.url.front() != '/') {
            req.url.insert(0, "/");
        }

        std::list<Route>::const_iterator route = routes.begin();
        std::list<Route>::const_iterator end = routes.end();

        while (route != end && next) {  // todo: to be exchanged by an stl-algorithm
            next = route->dispatch(cpath, req, res);
            ++route;
        }
    }
    return next;
}


bool MiddlewareRoute::dispatch(const MountPoint& mountPoint, const std::string& parentPath, const Request& req,
                               const Response& res) const {
    bool next = true;
    std::string cpath = path_concat(parentPath, mountPoint.path);
    
    if ((req.path.rfind(cpath, 0) == 0 && mountPoint.method == "use") ||
        (cpath == req.path && (req.method() == mountPoint.method || mountPoint.method == "all"))) {
        next = false;
        req.url = req.originalUrl.substr(cpath.length());
    
        if (req.url.front() != '/') {
            req.url.insert(0, "/");
        }

        this->dispatcher(req, res, [&next](void) -> void {
            next = true;
        });
    }
    return next;
}


bool DispatcherRoute::dispatch(const MountPoint& mountPoint, const std::string& parentPath, const Request& req,
                               const Response& res) const {
    bool next = true;
    std::string cpath = path_concat(parentPath, mountPoint.path);
    
    if (cpath == req.path && (req.method() == mountPoint.method || mountPoint.method == "all")) {
        req.url = req.originalUrl.substr(cpath.length());
        
        if (req.url.front() != '/') {
            req.url.insert(0, "/");
        }

        this->dispatcher(req, res);
        
        next = false;
    }

    return next;
}


bool Router::dispatch(const Request& req, const Response& res) const {
    return routerRoute->dispatch(mountPoint, "/", req, res);
}
