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


bool RRoute::dispatch(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;
    
    std::string& cpath = path_concat(mpath, path);
    
    if (request.path.rfind(cpath, 0) == 0) {
        next = router.dispatch(method, cpath, request, response);
    }
    
    return next;
}


bool PRoute::dispatch(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;
    
    std::string& cpath = path_concat(mpath, path);
    
    if (cpath == request.path) {
        this->dispatcher(request, response);
        next = false;
    }
    
    return next;
}


bool MRoute::dispatch(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;
    
    std::string& cpath = path_concat(mpath, path);
    
    if (cpath == request.path) {
        next = false;
        this->dispatcher(request, response, [&next] (void) -> void {
            next = true;
        });
    }
    
    return next;
}


Router::~Router() {
    std::map<const std::string, std::list<const Route*>>::iterator it;
    for (it = mroutes.begin(); it != mroutes.end(); ++it) {
        std::list<const Route*>::const_iterator itb = it->second.begin();
        std::list<const Route*>::const_iterator ite = it->second.end();
        
        while(itb != ite) {
            delete *itb;
        }
    }
    
    for (it = routes.begin(); it != routes.end(); ++it) {
        std::list<const Route*>::const_iterator itb = it->second.begin();
        std::list<const Route*>::const_iterator ite = it->second.end();
        
        while(itb != ite) {
            delete *itb;
        }
    }
}


bool Router::dispatch(const std::map<const std::string, std::list<const Route*>>& routes, const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;
    
    std::map<const std::string, std::list<const Route*>>::const_iterator it = routes.find("use");
    if (it != routes.end()) {
        std::list<const Route*>::const_iterator itb = it->second.begin();
        std::list<const Route*>::const_iterator ite = it->second.end();
        
        while(itb != ite && next) {
            next = (*itb)->dispatch(method, path, request, response);
            ++itb;
        }
    }
    
    it = routes.find(method);
    if (it != routes.end()) {
        std::list<const Route*>::const_iterator itb = it->second.begin();
        std::list<const Route*>::const_iterator ite = it->second.end();
        
        while(itb != ite && next) {
            next = (*itb)->dispatch(method, path, request, response);
            ++itb;
        }
    }
    
    return next;
}


bool Router::dispatch(const std::string& method, const std::string& path, const Request& request, const Response& response) const {
    dispatch(mroutes, method, path, request, response);
    
    bool next = dispatch(routes, method, path, request, response);

    return next;
}
