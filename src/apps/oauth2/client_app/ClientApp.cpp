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

#include "express/legacy/in/WebApp.h"
#include "express/middleware/StaticMiddleware.h"
#include "log/Logger.h"

#include <string>

int main(int argc, char* argv[]) {
    express::WebApp::init(argc, argv);
    const express::legacy::in::WebApp app("OAuth2Client");

    app.get("/oauth2", [] APPLICATION(req, res) {
        // if (req.query("grant_type")) {}
        if (!req->query("code").empty()) {
            res->sendFile("/home/rathalin/projects/snode.c/src/oauth2/client_app/vue-frontend-oauth2-client/dist/index.html",
                          [req](int ret) {
                              if (ret != 0) {
                                  PLOG(ERROR) << req->url;
                              }
                          });
            /*
            std::string tokenRequestUri{"http://localhost:8082/oauth2/token"};
            tokenRequestUri += "?grant_type=authorization_code";
            tokenRequestUri += "&code=" + req.query("code");
            if (!req.query("state").empty()) {
                tokenRequestUri += "&state=" + req.query("state");
            }
            tokenRequestUri += "&client_id=911a821a-ea2d-11ec-8e2e-08002771075f";
            tokenRequestUri += "&redirect_uri=http://localhost:8081/oauth2";
            VLOG(1) << "Recieving auth code from auth server: " << req.query("code") << ", requesting token from " << tokenRequestUri;
            res.redirect(tokenRequestUri);
            */
        }
    });

    app.use(express::middleware::StaticMiddleware("/home/rathalin/projects/snode.c/src/oauth2/client_app/vue-frontend-oauth2-client/dist"));

    app.listen(8081, [](const express::legacy::in::WebApp::SocketAddress& socketAddress, const core::socket::State& state) {
        switch (state) {
            case core::socket::State::OK:
                VLOG(1) << "OAuth2Client: connected to '" << socketAddress.toString() << "'";
                break;
            case core::socket::State::DISABLED:
                VLOG(1) << "OAuth2Client: disabled";
                break;
            case core::socket::State::ERROR:
                VLOG(1) << "OAuth2Client: error occurred";
                break;
            case core::socket::State::FATAL:
                VLOG(1) << "OAuth2Client: fatal error occurred";
                break;
        }
    });

    return express::WebApp::start();
}
