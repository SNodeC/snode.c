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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "WebApp.h"

#include "socket/legacy/SocketServer.h"

namespace express::legacy {

    WebApp::WebApp()
        : httpServer(
              [this]([[maybe_unused]] net::socket::legacy::SocketConnection* socketConnection) -> void { // onConnect
                  if (_onConnect != nullptr) {
                      _onConnect(socketConnection);
                  }
              },
              [this](http::Request& req, http::Response& res) -> void { // onRequestReady
                  dispatch(req, res);
              },
              []([[maybe_unused]] http::Request& req, [[maybe_unused]] http::Response& res) -> void { // onResponseFinished
              },
              [this]([[maybe_unused]] net::socket::legacy::SocketConnection* socketConnection) -> void { // onDisconnect
                  if (_onDisconnect != nullptr) {
                      _onDisconnect(socketConnection);
                  }
              }) {
    }

    void WebApp::listen(in_port_t port, const std::function<void(int err)>& onError) {
        httpServer.listen(port, onError);
    }

} // namespace express::legacy
