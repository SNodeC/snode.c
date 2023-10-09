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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "express/legacy/in/WebApp.h"
#include "express/middleware/StaticMiddleware.h"
#include "express/tls/in/WebApp.h"
#include "log/Logger.h"
#include "utils/Config.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    utils::Config::add_string_option("--web-root", "Root directory of the web site", "[path]");

    express::WebApp::init(argc, argv);

    using LegacyWebApp = express::legacy::in::WebApp;
    using LegacySocketAddress = LegacyWebApp::SocketAddress;

    LegacyWebApp legacyApp;
    legacyApp.getConfig().setReuseAddress();

    legacyApp.use(express::middleware::StaticMiddleware(utils::Config::get_string_option_value("--web-root")));

    legacyApp.listen(8080, [](const LegacySocketAddress& socketAddress, int errnum) {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << "snode.c listening on " << socketAddress.toString();
        }
    });

    using TLSWebApp = express::tls::in::WebApp;
    using TLSSocketAddress = TLSWebApp::SocketAddress;

    TLSWebApp tlsApp;
    tlsApp.getConfig().setReuseAddress();

    tlsApp.getConfig().setCertChain("/home/voc/projects/snodec/snode.c/certs/wildcard.home.vchrist.at_-_snode.c_-_server.pem");
    tlsApp.getConfig().setCertKey("/home/voc/projects/snodec/snode.c/certs/Volker_Christian_-_Web_-_snode.c_-_server.key.encrypted.pem");
    tlsApp.getConfig().setCertKeyPassword("snode.c");

    tlsApp.use(express::middleware::StaticMiddleware(utils::Config::get_string_option_value("--web-root")));

    tlsApp.listen(8088, [](const TLSSocketAddress& socketAddress, int errnum) {
        if (errnum < 0) {
            PLOG(ERROR) << "OnError";
        } else if (errnum > 0) {
            PLOG(ERROR) << "OnError: " << socketAddress.toString();
        } else {
            VLOG(0) << "snode.c listening on " << socketAddress.toString();
        }
    });

    return express::WebApp::start();
}
