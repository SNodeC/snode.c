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

#ifndef WEB_WEBSOCKET_SERVER_SOCKTECONTEXT_H
#define WEB_WEBSOCKET_SERVER_SOCKTECONTEXT_H

#include "web/websocket/SocketContextUpgrade.h"
#include "web/websocket/server/SubProtocol.h"

namespace core::socket::stream {
    class SocketConnection;
} // namespace core::socket::stream

namespace web {
    namespace http::server {
        class Request;
        class Response;
    } // namespace http::server
    namespace websocket {
        template <typename SubProtocolT>
        class SubProtocolFactory;

    } // namespace websocket
} // namespace web

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::server {

    class SocketContextUpgrade
        : public web::websocket::SocketContextUpgrade<SubProtocol, web::http::server::Request, web::http::server::Response> {
    public:
        SocketContextUpgrade(
            core::socket::stream::SocketConnection* socketConnection,
            web::http::SocketContextUpgradeFactory<web::http::server::Request, web::http::server::Response>* socketContextUpgradeFactory);

        ~SocketContextUpgrade() override;

        std::string loadSubProtocol(const std::list<std::string>& subProtocolNames);

    private:
        web::websocket::SubProtocolFactory<SubProtocol>* subProtocolFactory = nullptr;
    };

} // namespace web::websocket::server

extern template class web::http::SocketContextUpgradeFactory<web::http::server::Request, web::http::server::Response>;

#endif // WEB_WEBSOCKET_SERVER_SOCKTECONTEXT_H
