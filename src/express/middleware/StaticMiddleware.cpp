/*
 * SNode.C - a slim toolkit for network communication
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

#include "express/middleware/StaticMiddleware.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::middleware {

    StaticMiddleware::StaticMiddleware(const std::string& root)
        : root(root) {
        use(
            [&stdHeaders = this->stdHeaders, &stdCookies = this->stdCookies, &connectionState = this->defaultConnectionState] MIDDLEWARE(
                req, res, next) {
                if (req->method == "GET") {
                    if (connectionState == web::http::ConnectionState::Close) {
                        res->set("Connection", "close");
                    } else if (connectionState == web::http::ConnectionState::Keep) {
                        res->set("Connection", "keep-alive");
                    }
                    res->set(stdHeaders);
                    for (auto& [value, options] : stdCookies) {
                        res->cookie(value, options.getValue(), options.getOptions());
                    }
                    next();
                } else {
                    LOG(INFO) << "Express StaticMiddleware: Wrong method " << req->method;
                    if (connectionState == web::http::ConnectionState::Close) {
                        res->set("Connection", "close");
                    } else if (connectionState == web::http::ConnectionState::Keep) {
                        res->set("Connection", "keep-alive");
                    }
                    res->sendStatus(405);
                }
            },
            [] MIDDLEWARE(req, res, next) {
                if (req->url == "/") {
                    LOG(INFO) << "Express StaticMiddleware: REDIRECTING " + req->url + " -> " + "/index.html";
                    res->redirect(308, "/index.html");
                } else {
                    next();
                }
            },
            [&root = this->root] APPLICATION(req, res) {
                res->sendFile(root + req->url, [&root, req, res](int ret) {
                    PLOG(INFO) << "Express StaticMiddleware: GET " + req->url + " -> " + root + req->url;

                    if (ret != 0) {
                        res->status(404).end();
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

    class StaticMiddleware& StaticMiddleware::afterResponse(web::http::ConnectionState connectionState) {
        this->defaultConnectionState = connectionState;

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
