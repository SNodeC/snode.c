/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020 Volker Christian <me@vchrist.at>
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

#ifndef LEGACY_WEBAPP_H
#define LEGACY_WEBAPP_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <map>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "../WebApp.h"
#include "legacy/HTTPServer.h"

namespace express::legacy {

    class WebApp : public express::WebApp {
    public:
        explicit WebApp(const std::map<std::string, std::any>& options = {{}});

        WebApp& operator=(const express::WebApp& webApp) = delete;

        void listen(in_port_t port, const std::function<void(int err)>& onError = nullptr) override;
        void listen(const std::string& host, in_port_t port, const std::function<void(int err)>& onError = nullptr) override;

        void onConnect(const std::function<void(net::socket::legacy::SocketConnection*)>& onConnect) {
            _onConnect = onConnect;
        }

        void onDisconnect(const std::function<void(net::socket::legacy::SocketConnection*)>& onDisconnect) {
            _onDisconnect = onDisconnect;
        }

    protected:
        std::function<void(net::socket::legacy::SocketConnection*)> _onConnect = nullptr;
        std::function<void(net::socket::legacy::SocketConnection*)> _onDisconnect = nullptr;

        http::legacy::HTTPServer httpServer;

    private:
        using express::WebApp::start;
        using express::WebApp::stop;
    };

} // namespace express::legacy

#endif // LEGACY_WEBAPP_H
