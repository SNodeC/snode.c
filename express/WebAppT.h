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

#ifndef WEBAPPT_H
#define WEBAPPT_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <any>
#include <functional>
#include <map>
#include <netinet/in.h>
#include <string> // for string

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "WebApp.h"

namespace express {

    template <typename HTTPServerT>
    class WebAppT : public WebApp {
    public:
        using HTTPServer = HTTPServerT;

        explicit WebAppT(const std::map<std::string, std::any>& options = {{}})
            : httpServer(
                  [this]([[maybe_unused]] typename HTTPServer::SocketServer::SocketConnection* socketConnection) -> void { // onConnect
                      if (_onConnect != nullptr) {
                          _onConnect(socketConnection);
                      }
                  },
                  [this](http::Request& req, http::Response& res) -> void { // onRequestReady
                      dispatch(req, res);
                  },
                  [this](http::Request& req, http::Response& res) -> void { // onRequestCompleted
                      completed(req, res);
                  },
                  [this]([[maybe_unused]] typename HTTPServer::SocketServer::SocketConnection* socketConnection) -> void { // onDisconnect
                      if (_onDisconnect != nullptr) {
                          _onDisconnect(socketConnection);
                      }
                  },
                  options) {
        }

        WebAppT& operator=(const express::WebAppT<HTTPServer>& webApp) = delete;

        void listen(in_port_t port, const std::function<void(int err)>& onError = nullptr) override {
            httpServer.listen(port, onError);
        }

        void listen(const std::string& host, in_port_t port, const std::function<void(int err)>& onError = nullptr) override {
            httpServer.listen(host, port, onError);
        }

        void onConnect(const std::function<void(typename HTTPServer::SocketServer::SocketConnection*)>& onConnect) {
            _onConnect = onConnect;
        }

        void onDisconnect(const std::function<void(typename HTTPServer::SocketServer::SocketConnection*)>& onDisconnect) {
            _onDisconnect = onDisconnect;
        }

    protected:
        std::function<void(typename HTTPServer::SocketServer::SocketConnection*)> _onConnect = nullptr;
        std::function<void(typename HTTPServer::SocketServer::SocketConnection*)> _onDisconnect = nullptr;

        HTTPServer httpServer;

    private:
        using express::WebApp::init;
        using express::WebApp::start;
        using express::WebApp::stop;
    };

} // namespace express

#endif // WEBAPPT_H
