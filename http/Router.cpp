#include "Router.h"
#include "Request.h"
#include "Response.h"
#include <regex>
#include <iostream>

#define PATH_REGEX ":[a-zA-Z0-9]+(\\(.+?\\))?"

static const std::string path_concat(const std::vector<std::string>& stringvec) {
    std::string s;
    for (int i = 0; i < stringvec.size(); i++) {
        if (!stringvec[i].empty() && stringvec[i].front() != ' ') {
            s += "\\/" + stringvec[i];
        }
    }
    return s;
}

static const std::vector<std::string> explode(std::string const & s, char delim)
{
    std::vector<std::string> result;
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, delim); )
    {
        result.push_back(std::move(token));
    }

    return result;
}

static const std::smatch matchResult(const std::string& cpath) {
    std::smatch smatch;
    std::regex_search(cpath, smatch, std::regex(PATH_REGEX));
    return smatch;
}

static const bool hasResult(const std::string& cpath) {
    std::smatch smatch;
    return std::regex_search(cpath, smatch, std::regex(PATH_REGEX));
}


static const bool matchFunction(const std::string& cpath, const std::string& reqpath) {
    std::vector<std::string> explodedString = explode(cpath, '/');
    
    for(int i = 0; i < explodedString.size(); i++) {
        if (explodedString[i].front() == ':') {
            std::smatch smatch = matchResult(explodedString[i]);
            std::string regex = "(.*)";
            if(smatch.size() > 1) {
                if(smatch[1] != "") {
                  regex = smatch[1];
                }
            } 
            explodedString[i] = regex;
        }
    }
    
    std::string regexPath = path_concat(explodedString);
    return std::regex_match(reqpath, std::regex(regexPath));
}

static const void setParams(const std::string& cpath, const Request& req) {
    std::vector<std::string> explodedString = explode(cpath, '/');
    std::vector<std::string> explodedReqString = explode(req.originalUrl, '/');
    
    for(int i = 0; i < explodedString.size(); i++) {
        if (explodedString[i].front() == ':') {
            std::smatch smatch = matchResult(explodedString[i]);
            std::string regex = "(.*)";
            if(smatch.size() > 1) {
                if(smatch[1] != "") {
                  regex = smatch[1];
                }
            } 
            
            if(std::regex_match(explodedReqString[i], std::regex(regex))) {
                std::string attributeName = smatch[0];
                attributeName.erase(0, 1);
                attributeName.erase((attributeName.length()-smatch[1].length()), smatch[1].length());
                
                req.setAttribute<std::string>(explodedReqString[i], attributeName);
            }
        }
    }
}

static const bool checkForUrlMatch(const std::string& cpath, const std::string& reqpath) {
    bool hasRegex = hasResult(cpath);
    return (!hasRegex && cpath == reqpath) || (hasRegex && matchFunction(cpath, reqpath));
}

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
    
    if (request.path.rfind(cpath, 0) == 0 && method == this->method) {
        request.url = request.originalUrl.substr(cpath.length());
        if (request.url.front() != '/') {
            request.url.insert(0, "/");
        }
    
        next = router.dispatch(method, cpath, request, response);
    }

    return next;
}


bool DispatcherRoute::dispatch(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;

    std::string cpath = path_concat(mpath, path);
    
    if ((request.path.rfind(cpath, 0) == 0 && this->method == "use") || (checkForUrlMatch(cpath, request.originalUrl) && (method == this->method || this->method == "all"))) {
        // request.url = request.originalUrl.substr(cpath.length());
        /* TODO: change to substr */
        request.url = request.originalUrl;
        if (request.url.front() != '/') {
            request.url.insert(0, "/");
        }
        
        if(hasResult(cpath)) {
            setParams(cpath, request);
        }
        
        this->dispatcher(request, response);
        next = false;
    }

    return next;
}


bool MiddlewareRoute::dispatch(const std::string& method, const std::string& mpath, const Request& request, const Response& response) const {
    bool next = true;

    std::string cpath = path_concat(mpath, path);
    
    if ((request.path.rfind(cpath, 0) == 0 && this->method == "use") || (checkForUrlMatch(cpath, request.originalUrl) && (method == this->method || this->method == "all"))) {
        next = false;
        // request.url = request.originalUrl.substr(cpath.length());
        /* TODO: change to substr */
        request.url = request.originalUrl;
        if (request.url.front() != '/') {
            request.url.insert(0, "/");
        }
        
        if(hasResult(cpath)) {
            setParams(cpath, request);
        }
        
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
        next = (*itb)->dispatch(method, mpath, request, response);
        ++itb;
    }

    return next;
}

bool Router::dispatch(const std::string& method, const std::string& path, const Request& request, const Response& response) const {
    bool next = dispatch(routes, method, path, request, response);

    return next;
}
