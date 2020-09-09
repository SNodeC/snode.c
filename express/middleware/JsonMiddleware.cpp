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
#include <./json.hpp>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "Request.h"
#include "Response.h"
#include "JsonMiddleware.h"

using namespace express;
using json = nlohmann::json;

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
            // convert req.body char* to string
            std::string s(req.body, req.contentLength);
            VLOG(0) << "JsonMiddleware is parsing body: " << s;

            // parse body string with json library
            // store it as type json from nlohmann library
            json j = json::parse(s);

            // set all the json data as attributes in the request object
            req.setAttribute<json>(j);

            next();
        }
    });
}

// Keep the created json middleware alive
static std::shared_ptr<class JsonMiddleware> jsonMiddleware = nullptr;

const class JsonMiddleware& JsonMiddleware::instance(const std::string& root) {
    if (jsonMiddleware==nullptr) {
        jsonMiddleware = std::shared_ptr<JsonMiddleware>(new JsonMiddleware(root));
    }

    return *jsonMiddleware;
}

// "Constructor" of JsonMiddleware
const class JsonMiddleware& JsonMiddleware(const std::string& root) {
    return JsonMiddleware::instance(root);
}
