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

#include "core/SNodeC.h"
#include "express/legacy/in/WebApp.h"
#include "log/Logger.h"

#include <iostream>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <string>

using express::Request;
using express::Response;
using express::Router;

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    const express::legacy::in::WebApp app("app");

    // Global middleware (prefix)
    app.use("/", [] MIDDLEWARE(req, res, next) {
        std::cout << "global.use -> " << req->method << " " << req->originalUrl << "\n";
        next();
    });

    // Health (end-anchored)
    app.get("/health", [] APPLICATION(req, res) {
        res->json({{"ok", true}});
    });

    // Query-constrained route: /search?q=books
    app.get("/search", [] MIDDLEWARE(req, res, next) {
        if (req->queries["q"] == "books") {
            res->json({{"search", "books"}});
            return;
        }
        next("route");
    });
    app.get("/search", [] APPLICATION(req, res) {
        res->status(404).json({{"error", "unsupported query"}});
    });

    // Static vs param ordering (static first)
    app.get("/users/list", [] APPLICATION(req, res) {
        res->send("users list (static wins if placed first)");
    });
    app.get("/users/:id(\\d+)", [] APPLICATION(req, res) {
        res->send("user #" + req->params["id"]);
    });

    // Nested router at /api
    Router api;

    // Guard: require admin=true else skip the whole router
    api.use([] MIDDLEWARE(req, res, next) {
        if (req->queries["admin"] != "true") {
            next("router");
            return;
        }
        next();
    });

    // Multi-handler chain + next('route') demo
    api.get(
        "/things/:id(\\d+)",
        [] MIDDLEWARE(req, res, next) {
            if (req->params["id"] == "0") {
                next("route");
                return;
            }
            next();
        },
        [] APPLICATION(req, res) {
            res->send("api thing " + req->params["id"]);
        });
    api.get("/things/:id(\\d+)", [] APPLICATION(req, res) {
        res->send("api thing fallback for id=0");
    });

    // Param on mount: /api/tenant/:tenant/info
    const Router tenant;
    tenant.get("/info", [] APPLICATION(req, res) {
        res->json({{"tenant", req->params["tenant"]}, {"info", true}});
    });
    api.use("/tenant/:tenant", tenant);

    // Admin stats
    api.get("/admin/stats", [] APPLICATION(req, res) {
        res->json({{"scope", "admin"}, {"ok", true}});
    });

    app.use("/api", api);

    // Strict vs lax demo (two explicit routes like in Node)
    app.get("/strict", [] APPLICATION(req, res) {
        res->send("strict-lax demo (no slash)");
    });
    app.get("/strict/", [] APPLICATION(req, res) {
        res->send("strict-lax demo (with slash)");
    });

    // HEAD→GET fallback: your patched method gate + Response::end() should handle HEAD
    app.get("/head-demo", [] APPLICATION(req, res) {
        res->set("X-Demo", "1");
        res->send("head body");
    });

    // Catch-all
    app.get("/:rest(.*)", [] APPLICATION(req, res) {
        res->status(404).send("not found: " + req->params["rest"]);
    });

    // Start server (adapt to your HTTP bootstrap)
    // express::WebAppT web("demo", app);
    // web.listen(8080);
    std::cout << "SNode.C Express on http://localhost:8080\n";

    app.listen(
        8080,
        []([[maybe_unused]] const express::legacy::in::WebApp::SocketAddress& socketAddr, [[maybe_unused]] core::socket::State state) {
            VLOG(0) << "EmptyWebapp listening";
        });

    return core::SNodeC::start();
}
