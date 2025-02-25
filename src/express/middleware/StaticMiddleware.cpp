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

#include "express/middleware/StaticMiddleware.h"

#include "core/socket/stream/SocketConnection.h"
#include "web/http/server/SocketContext.h"

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
                    LOG(DEBUG) << res->getSocketContext()->getSocketConnection()->getConnectionName()
                               << " Express StaticMiddleware correct method: " << req->method;

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
                    LOG(ERROR) << res->getSocketContext()->getSocketConnection()->getConnectionName()
                               << " Express StaticMiddleware wrong method: " << req->method;

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
                    LOG(INFO) << res->getSocketContext()->getSocketConnection()->getConnectionName()
                              << " Express StaticMiddleware Redirecting: " << req->url << " -> " << "/index.html'";
                    res->redirect(308, "/index.html");
                } else {
                    next();
                }
            },
            [&root = this->root] APPLICATION(req, res) {
                res->sendFile(root + req->url, [&root, req, res](int ret) {
                    if (ret == 0) {
                        LOG(INFO) << res->getSocketContext()->getSocketConnection()->getConnectionName()
                                  << " Express StaticMiddleware: GET " << req->url + " -> " << root + req->url;
                    } else {
                        PLOG(ERROR) << res->getSocketContext()->getSocketConnection()->getConnectionName() << " Express StaticMiddleware "
                                    << req->url + " -> " << root + req->url;

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
