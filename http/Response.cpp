#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "HTTPContext.h"
#include "HTTPStatusCodes.h"
#include "Response.h"
#include "httputils.h"


Response::Response(HTTPContext* httpContext)
    : httpContext(httpContext) {
}


const Response& Response::status(int status) const {
    this->responseStatus = status;

    return *this;
}


const Response& Response::append(const std::string& field, const std::string& value) const {
    std::map<std::string, std::string>::iterator it = this->responseHeader.find(field);

    if (it != this->responseHeader.end()) {
        it->second += ", " + value;
    } else {
        this->set(field, value);
    }

    return *this;
}


const Response& Response::set(const std::map<std::string, std::string>& map) const {
    for (const std::pair<const std::string, std::string>& header : map) {
        this->set(header.first, header.second);
    }

    return *this;
}


const Response& Response::set(const std::string& field, const std::string& value) const {
    this->responseHeader.insert_or_assign(field, value);

    return *this;
}


const Response& Response::cookie(const std::string& name, const std::string& value,
                                 const std::map<std::string, std::string>& options) const {
    this->responseCookies.insert({name, ResponseCookie(value, options)});

    return *this;
}


const Response& Response::clearCookie(const std::string& name, const std::map<std::string, std::string>& options) const {
    std::map<std::string, std::string> opts = options;

    opts.erase("Max-Age");
    time_t time = 0;
    opts["Expires"] = httputils::to_http_date(gmtime(&time));

    this->cookie(name, "", opts);

    return *this;
}


void Response::send(const char* puffer, int n) const {
    this->httpContext->send(puffer, n);
}


void Response::send(const std::string& text) const {
    this->httpContext->send(text);
}


void Response::sendFile(const std::string& file, const std::function<void(int err)>& fn) const {
    this->httpContext->sendFile(file, fn);
}


void Response::download(const std::string& file, const std::function<void(int err)>& fn) const {
    std::string name = file;

    if (name[0] == '/') {
        name.erase(0, 1);
    }

    this->download(file, name, fn);
}


void Response::download(const std::string& file, const std::string& name, const std::function<void(int err)>& fn) const {
    this->set({{"Content-Disposition", "attachment; filename=\"" + name + "\""}});
    this->sendFile(file, fn);
}


void Response::redirect(const std::string& name) const {
    this->redirect(302, name);
}


void Response::redirect(int status, const std::string& name) const {
    this->status(status);
    this->set({{"Location", name}});
    this->end();
}


void Response::sendStatus(int status) const {
    this->status(status);
    this->send(HTTPStatusCode::reason(status));
}


const Response& Response::type(const std::string& type) const {
    this->set({{"Content-Type", type}});

    return *this;
}


void Response::end() const {
    this->httpContext->end();
}


void Response::reset() {
    responseStatus = 200;
    responseHeader.clear();
    responseCookies.clear();
}
