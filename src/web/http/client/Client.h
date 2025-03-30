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

#ifndef WEB_HTTP_CLIENT_CLIENT_H
#define WEB_HTTP_CLIENT_CLIENT_H

#include "web/http/client/SocketContextFactory.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace web::http::client {

    template <template <typename SocketContextFactoryT, typename... Args> typename SocketClientT>
    class Client
        : public SocketClientT<web::http::client::SocketContextFactory,
                               std::function<void(const std::shared_ptr<web::http::client::Request>&)>,
                               std::function<void(const std::shared_ptr<web::http::client::Request>&)>,
                               std::function<net::config::ConfigInstance&()>> {
    public:
        using Request = web::http::client::Request;
        using Response = web::http::client::Response;

    private:
        using Super = SocketClientT<web::http::client::SocketContextFactory,
                                    std::function<void(const std::shared_ptr<web::http::client::Request>&)>,
                                    std::function<void(const std::shared_ptr<web::http::client::Request>&)>,
                                    std::function<net::config::ConfigInstance&()>>; // this makes it an HTTP client;

    public:
        using SocketConnection = typename Super::SocketConnection;
        using SocketAddress = typename Super::SocketAddress;

        Client(const std::string& name,
               const std::function<void(SocketConnection*)>& onConnect,
               const std::function<void(SocketConnection*)>& onConnected,
               const std::function<void(SocketConnection*)>& onDisconnect,
               std::function<void(const std::shared_ptr<Request>&)>&& onRequestBegin,
               std::function<void(const std::shared_ptr<Request>&)>&& onRequestEnd)
            : Super(name,
                    onConnect,
                    onConnected,
                    onDisconnect,
                    std::forward<std::function<void(const std::shared_ptr<Request>&)>>(onRequestBegin),
                    std::forward<std::function<void(const std::shared_ptr<Request>&)>>(onRequestEnd),
                    [this]() -> net::config::ConfigInstance& {
                        return Super::getConfig();
                    }) {
        }

        Client(const std::function<void(SocketConnection*)>& onConnect,
               const std::function<void(SocketConnection*)>& onConnected,
               const std::function<void(SocketConnection*)>& onDisconnect,
               std::function<void(const std::shared_ptr<Request>&)>&& onRequestBegin,
               std::function<void(const std::shared_ptr<Request>&)>&& onRequestEnd)
            : Client("",
                     onConnect,
                     onConnected,
                     onDisconnect,
                     std::forward<std::function<void(const std::shared_ptr<Request>&)>>(onRequestBegin),
                     std::forward<std::function<void(const std::shared_ptr<Request>&)>>(onRequestEnd)) {
        }

        Client(const std::string& name,
               std::function<void(const std::shared_ptr<Request>&)>&& onRequestBegin,
               std::function<void(const std::shared_ptr<Request>&)>&& onRequestEnd)
            : Super(name,
                    std::forward<std::function<void(const std::shared_ptr<Request>&)>>(onRequestBegin),
                    std::forward<std::function<void(const std::shared_ptr<Request>&)>>(onRequestEnd),
                    [this]() -> net::config::ConfigInstance& {
                        return Super::getConfig();
                    }) {
        }

        Client(std::function<void(const std::shared_ptr<Request>&)>&& onRequestBegin,
               std::function<void(const std::shared_ptr<Request>&)>&& onRequestEnd)
            : Client("",
                     std::forward<std::function<void(const std::shared_ptr<Request>&)>>(onRequestBegin),
                     std::forward<std::function<void(const std::shared_ptr<Request>&)>>(onRequestEnd)) {
        }

        void setPipelinedRequests(bool pipelinedRequests) {
            Super::getSocketContextFactory()->setPipelinedRequests(pipelinedRequests);
        }
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_CLIENT_H
