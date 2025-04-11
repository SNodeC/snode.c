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

#include <string>

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    const express::legacy::in::WebApp emptyWebapp("empty");
    const express::legacy::in::WebApp routeWebapp("route");

    const express::Router emptyRouter;
    const express::Router routeRouter;

    emptyRouter.get([] APPLICATION(req, res) {
        VLOG(0) << "EmptyRoute Matches: " << req->originalUrl;
    });

    routeRouter.get("/route", [] APPLICATION(req, res) {
        VLOG(0) << "RoouteRoute Matches: " << req->originalUrl;
    });

    emptyRouter.get(emptyRouter);
    emptyRouter.get(routeRouter);

    routeRouter.get("/route", emptyRouter);
    routeRouter.get("/route", routeRouter);

    emptyWebapp.get(emptyRouter);
    emptyWebapp.get(routeRouter);

    routeWebapp.get("/route", emptyRouter);
    routeWebapp.get("/route", routeRouter);

    emptyWebapp.listen(
        8080,
        []([[maybe_unused]] const express::legacy::in::WebApp::SocketAddress& socketAddr, [[maybe_unused]] core::socket::State state) {
            VLOG(0) << "EmptyWebapp listening";
        });

    routeWebapp.listen(
        8081,
        []([[maybe_unused]] const express::legacy::in::WebApp::SocketAddress& socketAddr, [[maybe_unused]] core::socket::State state) {
            VLOG(0) << "RouteWebapp listening";
        });

    return core::SNodeC::start();
}
