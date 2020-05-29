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


bool Route::dispatch(const std::string& method, const std::string& parentPath, const Request& req, const Response& res) const {
    return route->dispatch(mountPoint, method, parentPath, req, res);
}


bool RouterRoute::dispatch(const MountPoint& mountPoint, const std::string& method, const std::string& parentPath, const Request& req,
                           const Response& res) const {
    bool next = true;
    std::string cpath = path_concat(parentPath, mountPoint.path);
/*
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::cout << "    Request Method: " << method << std::endl;
    std::cout << "    Request Path: " << req.path << std::endl;
    std::cout << "    Registered For: " << mountPoint.method << std::endl;
    std::cout << "    Parent Path: " << parentPath << std::endl;
    std::cout << "    Registered Path: " << mountPoint.path << std::endl;
    std::cout << "    Combined Path: " << cpath << std::endl;
    std::cout << "    Path Match: " << req.path.rfind(cpath, 0) << std::endl;
*/
    if (req.path.rfind(cpath, 0) == 0 && (mountPoint.method == method || mountPoint.method == "use")) {
/*
        std::cout << "    -----" << std::endl;
        std::cout << "    MATCH" << std::endl;
        std::cout << "    -----" << std::endl;
*/
        req.url = req.originalUrl.substr(cpath.length());
        if (req.url.front() != '/') {
            req.url.insert(0, "/");
        }

        std::list<Route>::const_iterator route = routes.begin();
        std::list<Route>::const_iterator end = routes.end();

//         std::cout << ">>>>>>>" << std::endl;
        while (route != end && next) {
            next = route->dispatch(method, cpath, req, res);
            ++route;
        }
//         std::cout << "<<<<<<<" << std::endl;
    }
    return next;
}


bool MiddlewareRoute::dispatch(const MountPoint& mountPoint, const std::string& method, const std::string& parentPath, const Request& req,
                               const Response& res) const {
    bool next = true;
    std::string cpath = path_concat(parentPath, mountPoint.path);
/*
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::cout << "    Request Method: " << method << std::endl;
    std::cout << "    Request Path: " << req.path << std::endl;
    std::cout << "    Registered For: " << mountPoint.method << std::endl;
    std::cout << "    Parent Path: " << parentPath << std::endl;
    std::cout << "    Registered Path: " << mountPoint.path << std::endl;
    std::cout << "    Combined Path: " << cpath << std::endl;
    std::cout << "    Path Match: " << req.path.rfind(cpath, 0) << std::endl;
*/
    if ((req.path.rfind(cpath, 0) == 0 && mountPoint.method == "use") ||
        (cpath == req.path && (method == mountPoint.method || mountPoint.method == "all"))) {
/*
        std::cout << "    -----" << std::endl;
        std::cout << "    MATCH" << std::endl;
        std::cout << "    -----" << std::endl;
*/
        next = false;
        req.url = req.originalUrl.substr(cpath.length());
        if (req.url.front() != '/') {
            req.url.insert(0, "/");
        }

//         std::cout << ">>>>>>>" << std::endl;
        this->dispatcher(req, res, [&next](void) -> void {
            next = true;
        });
//         std::cout << "<<<<<<<" << std::endl;
    }
    return next;
}


bool DispatcherRoute::dispatch(const MountPoint& mountPoint, const std::string& method, const std::string& parentPath, const Request& req,
                               const Response& res) const {
    bool next = true;
    std::string cpath = path_concat(parentPath, mountPoint.path);
/*
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    std::cout << "    Request Method: " << method << std::endl;
    std::cout << "    Request Path: " << req.path << std::endl;
    std::cout << "    Registered For: " << mountPoint.method << std::endl;
    std::cout << "    Parent Path: " << parentPath << std::endl;
    std::cout << "    Registered Path: " << mountPoint.path << std::endl;
    std::cout << "    Combined Path: " << cpath << std::endl;
    std::cout << "    Path Match: " << (cpath == req.path) << std::endl;
*/
    if (cpath == req.path && (method == mountPoint.method || mountPoint.method == "all")) {
/*
        std::cout << "    -----" << std::endl;
        std::cout << "    MATCH" << std::endl;
        std::cout << "    -----" << std::endl;
*/
        req.url = req.originalUrl.substr(cpath.length());
        if (req.url.front() != '/') {
            req.url.insert(0, "/");
        }

//         std::cout << ">>>>>>>" << std::endl;
        this->dispatcher(req, res);
//         std::cout << "<<<<<<<" << std::endl;
        next = false;
    }

    return next;
}


bool Router::dispatch(const std::string& method, const Request& req, const Response& res) const {
    return routerRoute->dispatch(mountPoint, method, "/", req, res);
}
