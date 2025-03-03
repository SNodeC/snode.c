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

#ifndef WEB_HTTP_CLIENT_SOCKETCONTEXT_H
#define WEB_HTTP_CLIENT_SOCKETCONTEXT_H

#include "core/socket/stream/SocketContext.h" // IWYU pragma: export
#include "web/http/client/Request.h"          // IWYU pragma: export
#include "web/http/client/ResponseParser.h"   // IWYU pragma: export

namespace core::socket::stream {
    class SocketConnection;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <functional>
#include <list>
#include <memory> // IWYU pragma: export
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::http::client {

    class SocketContext : public core::socket::stream::SocketContext {
    private:
        using Super = core::socket::stream::SocketContext;

        using Request = web::http::client::Request;
        using Response = web::http::client::Response;

    public:
        SocketContext(core::socket::stream::SocketConnection* socketConnection,
                      const std::function<void(const std::shared_ptr<Request>&)>& onRequestBegin,
                      const std::function<void(const std::shared_ptr<Request>&)>& onRequestEnd,
                      bool pipelinedRequests);

        ~SocketContext() override;

    private:
        void requestPrepared(Request&& request);
        void initiateRequest(Request& request);
        void requestDelivered(Request&& request, bool success);
        void responseStarted();
        void deliverResponse(Response&& response);
        void deliverResponseParseError(int status, const std::string& reason);
        void responseDelivered(bool httpClose);

        std::function<void(const std::shared_ptr<Request>&)> onRequestBegin;
        std::function<void(const std::shared_ptr<Request>&)> onRequestEnd;

        void onConnected() override;
        std::size_t onReceivedFromPeer() override;
        void onDisconnected() override;
        bool onSignal(int signum) override;
        void onWriteError(int errnum) override;

        std::list<Request> pendingRequests;
        std::list<Request> deliveredRequests;

        bool pipelinedRequests = false;

        std::shared_ptr<Request> currentRequest = nullptr;
        std::shared_ptr<Request> masterRequest;

        ResponseParser parser;

        enum Flags { //
            NONE = 0b00000000,
            HTTP10 = 0b00000001,
            HTTP11 = 0b00000010,
            KEEPALIVE = 0b00000100,
            CLOSE = 0b00001000
        };
        int flags = Flags::NONE;

        friend class web::http::client::Request;
    };

} // namespace web::http::client

#endif // WEB_HTTP_CLIENT_SOCKETCONTEXT_H
