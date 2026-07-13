/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include "SemanticLog.h"
#include "core/socket/stream/SocketConnection.h"
#include "web/http/http_utils.h"
#include "web/http/server/SocketContext.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <filesystem>
#include <map>
#include <optional>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace express::middleware {

    namespace {
        bool isRootOrDescendant(const std::filesystem::path& root, const std::filesystem::path& candidate) {
            auto rootIt = root.begin();
            auto candidateIt = candidate.begin();
            for (; rootIt != root.end(); ++rootIt, ++candidateIt) {
                if (candidateIt == candidate.end() || *rootIt != *candidateIt) {
                    return false;
                }
            }
            return true;
        }

        std::optional<std::filesystem::path> resolveStaticPath(const std::string& root, const std::string& requestPath) {
            // Containment is checked before sendFile(); this assumes the static root is not writable by untrusted local users
            // between resolution and open (TOCTOU), avoiding a broader platform-specific openat2() refactor here.
            std::string decodedPath;
            if (!httputils::url_decode(requestPath, decodedPath, httputils::UrlDecodeMode::Path) ||
                decodedPath.find('\0') != std::string::npos) {
                return std::nullopt;
            }

            std::error_code ec;
            const std::filesystem::path canonicalRoot = std::filesystem::weakly_canonical(std::filesystem::path(root), ec);
            if (ec) {
                return std::nullopt;
            }

            std::filesystem::path relative = decodedPath;
            if (relative.is_absolute()) {
                relative = relative.relative_path();
            }
            const std::filesystem::path candidate = std::filesystem::weakly_canonical(canonicalRoot / relative, ec);
            if (ec) {
                return std::nullopt;
            }

            if (!isRootOrDescendant(canonicalRoot, candidate)) {
                return std::nullopt;
            }

            return candidate;
        }
    } // namespace

    StaticMiddleware::StaticMiddleware(const std::string& root, bool fallThrough)
        : root(root)
        , index("index.html")
        , fallThrough(fallThrough) {
        setStrictRouting(false);

        use(
            [&stdHeaders = this->stdHeaders,
             &stdCookies = this->stdCookies,
             &connectionState = this->defaultConnectionState,
             &fallThrough = this->fallThrough] MIDDLEWARE(req, res, next) {
                snode::semantic::expressLog(*res->getSocketContext()->getSocketConnection()).debug() << "Express " << req->method;

                if (req->method != "GET") {
                    if (fallThrough) {
                        next("route");
                    } else {
                        res->sendStatus(405, "Unsupported method: " + req->method + "\n");
                    }
                } else {
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
                }
            },
            [&index = this->index] MIDDLEWARE(req, res, next) {
                if (req->path == "/") {
                    if (index.empty()) {
                        res->status(404).send("Unsupported resource: " + req->url + "\n");
                    } else {
                        snode::semantic::expressLog(*res->getSocketContext()->getSocketConnection()).info()
                            << "Express StaticMiddleware Redirecting: " << req->url << " -> "
                            << req->originalPath +
                                   (!req->originalPath.empty() && req->originalPath.back() != '/' && index.front() != '/' ? "/" : "") +
                                   index;
                        res->redirect(
                            308,
                            req->originalPath +
                                (!req->originalPath.empty() && req->originalPath.back() != '/' && index.front() != '/' ? "/" : "") + index);
                    }
                } else {
                    next();
                }
            },
            [&root = this->root, &fallThrough = this->fallThrough] MIDDLEWARE(req, res, next) {
                const std::optional<std::filesystem::path> staticPath = resolveStaticPath(root, req->path);
                if (!staticPath) {
                    if (fallThrough) {
                        next();
                    } else {
                        res->status(404).send("Unsupported resource: " + req->url + "\n");
                    }
                    return;
                }
                const std::string resolvedPath = staticPath->string();
                res->sendFile(resolvedPath, [resolvedPath, req, res, &next, &fallThrough](int ret) {
                    if (ret == 0) {
                        snode::semantic::expressLog(*res->getSocketContext()->getSocketConnection()).info()
                            << "Express StaticMiddleware: GET " << req->url << " -> " << resolvedPath;
                    } else {
                        snode::semantic::sysError(
                            snode::semantic::expressLog(*res->getSocketContext()->getSocketConnection()), logger::LogLevel::Error, ret)
                            << "Express StaticMiddleware " << req->url << " -> " << resolvedPath;

                        if (fallThrough) {
                            next();
                        } else {
                            res->status(404).send("Unsupported resource: " + req->url + "\n");
                        }
                    }
                });
            });
    }

    class StaticMiddleware& StaticMiddleware::setIndex(const std::string& index) {
        this->index = index;

        return *this;
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

    class StaticMiddleware& StaticMiddleware::instance(const std::string& root, bool fallThrough) {
        // Keep all created static middlewares alive
        static std::map<const std::string, std::shared_ptr<class StaticMiddleware>> staticMiddlewares;

        if (!staticMiddlewares.contains(root)) {
            staticMiddlewares[root] = std::shared_ptr<StaticMiddleware>(new StaticMiddleware(root, fallThrough));
        }

        return *staticMiddlewares[root];
    }

    // "Constructor" of StaticMiddleware
    class StaticMiddleware& StaticMiddleware(const std::string& root, bool fallThrough) {
        return StaticMiddleware::instance(root, fallThrough);
    }

} // namespace express::middleware
