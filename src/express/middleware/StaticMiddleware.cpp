/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#include "express/middleware/StaticMiddleware.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <map>
#include <memory>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::middleware {

    StaticMiddleware::StaticMiddleware(const std::string& root)
        : root(root) {
        use(
            [&stdHeaders = this->stdHeaders, &stdCookies = this->stdCookies, &forceClose = this->forceClose] MIDDLEWARE(req, res, next) {
                if (req.method == "GET") {
                    if (forceClose) {
                        res.set("Connection", "Close");
                    } else {
                        res.set("Connection", "Keep-Alive");
                    }
                    res.set(stdHeaders);
                    for (auto& [value, options] : stdCookies) {
                        res.cookie(value, options.getValue(), options.getOptions());
                    }
                    next();
                } else {
                    LOG(INFO) << "express static middleware: Wrong method " << req.method;
                    res.set("Connection", "Close");
                    res.sendStatus(400);
                }
            },
            [] MIDDLEWARE(req, res, next) {
                if (req.url == "/") {
                    LOG(INFO) << "express static middleware: REDIRECTING to" + req.url + " -> " + "/index.html";
                    res.redirect(308, "/index.html");
                } else {
                    next();
                }
            },
            [&root = this->root] APPLICATION(req, res) {
                LOG(INFO) << "express static middleware: GET " + req.url + " -> " + root + req.url;
                res.sendFile(root + req.url, [&req, &res](int ret) -> void {
                    if (ret != 0) {
                        PLOG(INFO) << req.url;
                        res.status(404).end();
                    }
                });
            });
    }

    class StaticMiddleware& StaticMiddleware::clearStdHeaders() {
        this->stdHeaders.clear();

        return *this;
    }

    class StaticMiddleware& StaticMiddleware::setStdHeaders(const std::map<std::string, std::string>& stdHeaders) {
        this->stdHeaders = stdHeaders;

        return *this;
    }

    class StaticMiddleware& StaticMiddleware::appendStdHeaders(const std::map<std::string, std::string>& stdHeaders) {
        this->stdHeaders.insert(stdHeaders.begin(), stdHeaders.end());

        return *this;
    }

    class StaticMiddleware& StaticMiddleware::appendStdHeaders(const std::string& field, const std::string& value) {
        this->stdHeaders[field] = value;

        return *this;
    }

    class StaticMiddleware& StaticMiddleware::appendStdCookie(const std::string& name,
                                                              const std::string& value,
                                                              const std::map<std::string, std::string>& options) {
        this->stdCookies.insert({name, web::http::CookieOptions(value, options)});

        return *this;
    }

    class StaticMiddleware& StaticMiddleware::alwaysClose() {
        this->forceClose = true;

        return *this;
    }

    class StaticMiddleware& StaticMiddleware::instance(const std::string& root) {
        // Keep all created static middlewares alive
        static std::map<const std::string, std::shared_ptr<class StaticMiddleware>> staticMiddlewares;

        if (!staticMiddlewares.contains(root)) {
            staticMiddlewares[root] = std::shared_ptr<StaticMiddleware>(new StaticMiddleware(root));
        }

        return *staticMiddlewares[root];
    }

    // "Constructor" of StaticMiddleware
    class StaticMiddleware& StaticMiddleware(const std::string& root) {
        return StaticMiddleware::instance(root);
    }

} // namespace express::middleware
