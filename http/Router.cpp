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


bool RRoute::process(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;
    
    std::string& cpath = path_concat(mpath, path);
    
    if (request.path.rfind(cpath, 0) == 0) {
        next = router.process(method, cpath, request, response);
    }
    
    return next;
}


bool PRoute::process(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;
    
    std::string& cpath = path_concat(mpath, path);
    
    if (cpath == request.path) {
        this->processor(request, response);
        next = false;
    }
    
    return next;
}


bool MRoute::process(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;
    
    std::string& cpath = path_concat(mpath, path);
    
    if (cpath == request.path) {
        next = false;
        this->processor(request, response, [&next] (void) -> void {
            next = true;
        });
    }
    
    return next;
}


bool Router::process(const std::map<const std::string, std::list<const Route*>>& routes, const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;
    
    std::map<const std::string, std::list<const Route*>>::const_iterator it = routes.find("use");
    if (it != routes.end()) {
        std::list<const Route*>::const_iterator itb = it->second.begin();
        std::list<const Route*>::const_iterator ite = it->second.end();
        
        while(itb != ite && next) {
            next = (*itb)->process(method, path, request, response);
            ++itb;
        }
    }
    
    it = routes.find(method);
    if (it != routes.end()) {
        std::list<const Route*>::const_iterator itb = it->second.begin();
        std::list<const Route*>::const_iterator ite = it->second.end();
        
        while(itb != ite && next) {
            next = (*itb)->process(method, path, request, response);
            ++itb;
        }
    }
    
    return next;
}


bool Router::process(const std::string& method, const std::string& path, const Request& request, const Response& response) const {
    process(mroutes, method, path, request, response);
    
    bool next = process(routes, method, path, request, response);

    return next;
}
