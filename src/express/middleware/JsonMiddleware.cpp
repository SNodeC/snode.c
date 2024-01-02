/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023  Volker Christian <me@vchrist.at>
 * Json Middleware 2020, 2021 Marlene Mayr, Anna Moser, Matteo Prock, Eric Thalhammer
 * Github <MarleneMayr><moseranna><MatteoMatteoMatteo><peregrin-tuk>
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

#include "express/middleware/JsonMiddleware.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <initializer_list>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

// IWYU pragma: no_include <nlohmann/detail/exceptions.hpp>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::middleware {

    JsonMiddleware::JsonMiddleware() {
        use([] MIDDLEWARE(req, res, next) {
            try {
                // parse body string with json library
                // store it as type json from nlohmann library
                //                nlohmann::json json = nlohmann::json::parse(req.body, req.body + req.contentLength);

                req.body.push_back(0);
                const nlohmann::json json = nlohmann::json::parse(req.body);

                // set all the json data as attributes in the request object
                req.setAttribute<nlohmann::json>(json);
            } catch ([[maybe_unused]] const nlohmann::detail::parse_error& error) {
                // silently fail if body is not json
            }

            next();
        });
    }

    class JsonMiddleware& JsonMiddleware::instance() {
        // Keep the created json middleware alive
        static std::shared_ptr<class JsonMiddleware> jsonMiddleware = nullptr;

        if (jsonMiddleware == nullptr) {
            jsonMiddleware = std::shared_ptr<JsonMiddleware>(new JsonMiddleware());
        }

        return *jsonMiddleware;
    }

    // "Constructor" of JsonMiddleware
    class JsonMiddleware& JsonMiddleware() {
        return JsonMiddleware::instance();
    }

} // namespace express::middleware
