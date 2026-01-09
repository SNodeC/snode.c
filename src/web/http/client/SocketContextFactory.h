/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#ifndef WEB_HTTP_CLIENT_SOCKETCONTEXTFACTORY_H
#define WEB_HTTP_CLIENT_SOCKETCONTEXTFACTORY_H

#include "core/socket/stream//SocketContextFactory.h"
#include "web/http/client/ConfigHTTP.h"
#include "web/http/client/SocketContext.h" // IWYU pragma: export

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    class SocketContextFactory : public core::socket::stream::SocketContextFactory {
    public:
        using MasterRequest = web::http::client::MasterRequest;
        using Response = web::http::client::Response;

        SocketContextFactory(const std::function<void(const std::shared_ptr<MasterRequest>&)>& onHttpConnected,
                             const std::function<void(const std::shared_ptr<MasterRequest>&)>& onHttpDisconnected,
                             const std::function<net::config::ConfigInstance&()>& getConfigInstance);

        void setHostHeader(const std::string& hostHeader);
        std::string getHostHeader() const;

        void setPipelinedRequests(bool pipelinedRequests);

    private:
        core::socket::stream::SocketContext* create(core::socket::stream::SocketConnection* socketConnection) override;

        std::function<void(const std::shared_ptr<MasterRequest>&)> onHttpConnected;
        std::function<void(const std::shared_ptr<MasterRequest>&)> onHttpDisconnected;

        ConfigHTTP configHttp;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_SOCKETCONTEXTFACTORY_H
