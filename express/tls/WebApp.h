/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TLS_WEBAPP_H
#define TLS_WEBAPP_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "../WebApp.h"
#include "tls/HTTPServer.h"

namespace express::tls {

    class WebApp : public express::WebApp {
    public:
        explicit WebApp(const std::string& cert, const std::string& key, const std::string& password);

        WebApp& operator=(const express::WebApp& webApp) = delete;

        void listen(in_port_t port, const std::function<void(int err)>& onError = nullptr) override;

    protected:
        http::tls::HTTPServer httpServer;

    private:
        using express::WebApp::start;
        using express::WebApp::stop;
    };

} // namespace express::tls

#endif // TLS_WEBAPP_H
