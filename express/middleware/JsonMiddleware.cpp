/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <easylogging++.h>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "Response.h"
#include "JsonMiddleware.h"

using namespace express;

JsonMiddleware::JsonMiddleware(const std::string& root)
    : root(root) {
    use([] MIDDLEWARE(req, res, next) {
        if (req.method == "POST") {
            res.set("Connection", "Keep-Alive");
            res.cookie("CookieName", "CookieValue");
            next();
        } else {
            res.set("Connection", "Close").sendStatus(400);
        }
    });
    use([] MIDDLEWARE(req, res, next) {
        if (req.url == "/") {
            res.redirect(308, "/index.html");
        } else {

            std::string s(req.body, req.contentLength);
            // VLOG(0) << "body: " << req.body;
            VLOG(0) << "clength: " << std::to_string(req.contentLength);
            VLOG(0) << "json middleware should act here: " << s;

            // json library should parse req.body string
            // store it as type json from nlohmann library
            // then set all the json data as attributes in the request object
            // req.setAttribute<json : Json, "string" : cstring>(value : json);

            next();
        }
    });
    use([this] APPLICATION(req, res) {
        VLOG(0) << "POST " + req.url + " -> " + this->root + req.url;
        res.sendFile(this->root + req.url, [&req](int ret) -> void {
            if (ret != 0) {
                PLOG(ERROR) << req.url;
            }
        });
    });
}

// Keep all created json middlewares alive
static std::map<const std::string, std::shared_ptr<class JsonMiddleware>> jsonMiddlewares;

const class JsonMiddleware& JsonMiddleware::instance(const std::string& root) {
    if (!jsonMiddlewares.contains(root)) {
        jsonMiddlewares[root] = std::shared_ptr<JsonMiddleware>(new JsonMiddleware(root));
    }

    return *jsonMiddlewares[root];
}

// "Constructor" of JsonMiddleware
const class JsonMiddleware& JsonMiddleware(const std::string& root) {
    return JsonMiddleware::instance(root);
}
