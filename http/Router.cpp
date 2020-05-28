#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "Response.h"
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


bool RouterRoute::dispatch(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;

    std::string cpath = path_concat(mpath, path);
    /*
    std::cout << "Router OriginalUrl: " << request.originalUrl << std::endl;
    std::cout << "Router Path: " << path << std::endl;
    std::cout << "Router MPath: " << mpath << std::endl;
    std::cout << "Router CPath: " << cpath << std::endl;
    */
    if (request.path.rfind(cpath, 0) == 0 && method == this->method) {
        request.url = request.originalUrl.substr(cpath.length());
        if (request.url.front() != '/') {
            request.url.insert(0, "/");
        }
        next = router.dispatch(method, cpath, request, response);
    }

    return next;
}


bool DispatcherRoute::dispatch(const std::string& method, const std::string& mpath, const Request& request,
                               const Response& response) const {
    bool next = true;

    std::string cpath = path_concat(mpath, path);
    /*
    std::cout << "Dispatcher OriginalUrl: " << request.originalUrl << std::endl;
    std::cout << "Dispatcher Path: " << path << std::endl;
    std::cout << "Dispatcher MPath: " << mpath << std::endl;
    std::cout << "Dispatcher CPath: " << cpath << std::endl;
    */
    if ((request.path.rfind(cpath, 0) == 0 && this->method == "use") ||
        (cpath == request.path && (method == this->method || this->method == "all"))) {
        request.url = request.originalUrl.substr(cpath.length());
        if (request.url.front() != '/') {
            request.url.insert(0, "/");
        }
        this->dispatcher(request, response);
        next = false;
    }

    return next;
}


bool MiddlewareRoute::dispatch(const std::string& method, const std::string& mpath, const Request& request,
                               const Response& response) const {
    bool next = true;

    std::string cpath = path_concat(mpath, path);
    /*
    std::cout << "Middleware OriginalUrl: " << request.originalUrl << std::endl;
    std::cout << "Middleware Path: " << path << std::endl;
    std::cout << "Middleware MPath: " << mpath << std::endl;
    std::cout << "Middleware CPath: " << cpath << std::endl;
    */
    if ((request.path.rfind(cpath, 0) == 0 && this->method == "use") ||
        (cpath == request.path && (method == this->method || this->method == "all"))) {
        next = false;
        request.url = request.originalUrl.substr(cpath.length());
        if (request.url.front() != '/') {
            request.url.insert(0, "/");
        }
        this->dispatcher(request, response, [&next](void) -> void {
            next = true;
        });
    }

    return next;
}


Router::~Router() {
    std::list<const Route*>::const_iterator itb = routes.begin();
    std::list<const Route*>::const_iterator ite = routes.end();

    while (itb != ite) {
        delete *itb;
        ++itb;
    }
}


bool Router::dispatch(const std::list<const Route*>& nroutes, const std::string& method, const std::string& mpath, const Request& request,
                      const Response& response) const {
    bool next = true;

    std::for_each(nroutes.begin(), nroutes.end(), [&next, &method, &mpath, &request, &response](const Route* route) -> void {
        next = route->dispatch(method, mpath, request, response);
    });

    return next;
}


bool Router::dispatch(const std::string& method, const std::string& path, const Request& request, const Response& response) const {
    bool next = dispatch(routes, method, path, request, response);

    return next;
}
