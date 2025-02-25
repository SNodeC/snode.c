/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "express/middleware/JsonMiddleware.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

// IWYU pragma: no_include <nlohmann/detail/exceptions.hpp>
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::middleware {

    JsonMiddleware::JsonMiddleware() {
        use([] MIDDLEWARE(req, res, next) {
            try {
                // parse body string with json library
                // store it as type json from nlohmann library
                //                nlohmann::json json = nlohmann::json::parse(req.body, req.body + req.contentLength);

                //                req->body.push_back(0);
                const nlohmann::json json = nlohmann::json::parse(std::string(req->body.begin(), req->body.end()));

                // set all the json data as attributes in the request object
                req->setAttribute<nlohmann::json>(json);
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
