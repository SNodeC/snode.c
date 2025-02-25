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
#include "express/tls/in/WebApp.h"
#include "utils/Config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    utils::Config::addStringOption("--web-root", "Root directory of the web site", "[path]");

    express::WebApp::init(argc, argv);

    using LegacyWebApp = express::legacy::in::WebApp;
    using LegacySocketAddress = LegacyWebApp::SocketAddress;

    const LegacyWebApp legacyApp;
    legacyApp.getConfig().setReuseAddress();

    legacyApp.use(express::middleware::StaticMiddleware(utils::Config::getStringOptionValue("--web-root")));

    legacyApp.listen(8080,
                     [instanceName = legacyApp.getConfig().getInstanceName()](const LegacySocketAddress& socketAddress,
                                                                              const core::socket::State& state) {
                         switch (state) {
                             case core::socket::State::OK:
                                 VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
                                 break;
                             case core::socket::State::DISABLED:
                                 VLOG(1) << instanceName << " disabled";
                                 break;
                             case core::socket::State::ERROR:
                                 LOG(ERROR) << instanceName << " " << socketAddress.toString() << ": " << state.what();
                                 break;
                             case core::socket::State::FATAL:
                                 LOG(FATAL) << instanceName << " " << socketAddress.toString() << ": " << state.what();
                                 break;
                         }
                     });

    using TLSWebApp = express::tls::in::WebApp;
    using TLSSocketAddress = TLSWebApp::SocketAddress;

    const TLSWebApp tlsApp;
    tlsApp.getConfig().setReuseAddress();

    tlsApp.getConfig().setCert("/home/voc/projects/snodec/snode.c/certs/wildcard.home.vchrist.at_-_snode.c_-_server.pem");
    tlsApp.getConfig().setCertKey("/home/voc/projects/snodec/snode.c/certs/Volker_Christian_-_Web_-_snode.c_-_server.key.encrypted.pem");
    tlsApp.getConfig().setCertKeyPassword("snode.c");

    tlsApp.use(express::middleware::StaticMiddleware(utils::Config::getStringOptionValue("--web-root")));

    tlsApp.listen(
        8088,
        [instanceName = legacyApp.getConfig().getInstanceName()](const TLSSocketAddress& socketAddress, const core::socket::State& state) {
            switch (state) {
                case core::socket::State::OK:
                    VLOG(1) << instanceName << " listening on '" << socketAddress.toString() << "'";
                    break;
                case core::socket::State::DISABLED:
                    VLOG(1) << instanceName << " disabled";
                    break;
                case core::socket::State::ERROR:
                    LOG(ERROR) << instanceName << " " << socketAddress.toString() << ": " << state.what();
                    break;
                case core::socket::State::FATAL:
                    LOG(FATAL) << instanceName << " " << socketAddress.toString() << ": " << state.what();
                    break;
            }
        });

    return express::WebApp::start();
}
