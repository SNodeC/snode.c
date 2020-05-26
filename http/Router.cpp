
                                  #include "Router.h"

#include "Request.h"


static std::string& path_concat(const std::string& first, const std::string& second) {
    static std::string result;
    
    if (first.back() == '/' && second.front() == '/') {
        result = first + second.substr(1);
    } else if (first.back() != '/' && second.front() != '/') {
        result = first + '/' + second;
    } else {
        result = first + second;
    }
    
    return result;
}


bool RouterRoute::dispatch(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;
    
    std::string& cpath = path_concat(mpath, path);
    
    if (request.path.rfind(cpath, 0) == 0 && method == this->method) {
        next = router.dispatch(method, cpath, request, response);
    }
    
    return next;
}


bool DispatcherRoute::dispatch(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;
    
    std::string& cpath = path_concat(mpath, path);
    
    if (cpath == request.path && method == this->method) {
        this->dispatcher(request, response);
        next = false;
    }
    
    return next;
}


bool MiddlewareRoute::dispatch(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;
    
    std::string& cpath = path_concat(mpath, path);
    
    if (cpath == request.path && (method == this->method || this->method == "use")) {
        next = false;
        this->dispatcher(request, response, [&next] (void) -> void {
            next = true;
        });
    }
    
    return next;
}


Router::~Router() {
    std::list<const Route*>::const_iterator itb = routes.begin();
    std::list<const Route*>::const_iterator ite = routes.end();
    
    while(itb != ite) {
        delete *itb;
        ++itb;
    }
}


bool Router::dispatch(const std::list<const Route*>& nroutes, const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;
    
    std::list<const Route*>::const_iterator itb = nroutes.begin();
    std::list<const Route*>::const_iterator ite = nroutes.end();
    
    while(itb != ite && next) {
        next = (*itb)->dispatch(method, path, request, response);
        ++itb;
    }
    
    return next;
}
    

bool Router::dispatch(const std::string& method, const std::string& path, const Request& request, const Response& response) const {
    bool next = dispatch(routes, method, path, request, response);
    
    return next;
}
