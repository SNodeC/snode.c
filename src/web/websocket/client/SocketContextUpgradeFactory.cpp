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

#include "web/websocket/client/SocketContextUpgradeFactory.h"

#include "web/http/client/Request.h"
#include "web/http/client/Response.h"
#include "web/websocket/SubProtocolFactory.h"
#include "web/websocket/client/SocketContextUpgrade.h"
#include "web/websocket/client/SubProtocolFactorySelector.h"

namespace web::websocket::client {
    class SubProtocol;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/base64.h"

#include <unistd.h>
#include <utility>

#if !defined(HAVE_GETENTROPY)
#include <cstddef>
#include <sys/types.h>
#if defined(SYS_GETRANDOM)
#include <sys/syscall.h>
#else
#include <fcntl.h>
#endif
#endif

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::websocket::client {

#if !defined(HAVE_GETENTROPY)
    int getentropy(void* buf, size_t buflen);
    int getentropy(void* buf, size_t buflen) {
#ifdef SYS_GETRANDOM
        const ssize_t ret = syscall(SYS_getrandom, buf, buflen, 0);
#else
        // Fallback to /dev/urandom if necessary
        const int fd = open("/dev/urandom", O_RDONLY);
        if (fd == -1) {
            return -1;
        }
        const ssize_t ret = read(fd, buf, buflen);
        close(fd);
#endif
        return (ret == static_cast<ssize_t>(buflen)) ? 0 : -1;
    }
#endif

    template <typename... Args>
    void SocketContextUpgradeFactory<Args...>::prepare(http::client::Request& request) {
        unsigned char ebytes[16];
        getentropy(ebytes, 16);

        request.set("Sec-WebSocket-Key", base64::base64_encode(ebytes, 16));
        request.set("Sec-WebSocket-Version", "13");
    }

    template <typename... Args>
    std::string SocketContextUpgradeFactory<Args...>::name() {
        return "websocket";
    }

    template <typename... Args>
    SubProtocol* SocketContextUpgradeFactory<Args...>::loadSubProtocol(const std::string& subProtocolName, Args&&... args) {
        SubProtocol* subProtocol = nullptr;

        web::websocket::SubProtocolFactory<SubProtocol>* subProtocolFactory =
            SubProtocolFactorySelector::instance()->select(subProtocolName, SubProtocolFactorySelector::Role::CLIENT);

        if (subProtocolFactory != nullptr) {
            subProtocol = subProtocolFactory->createSubProtocol(std::forward(args)...);
        }

        return subProtocol;
    }

    template <typename... Args>
    http::SocketContextUpgrade<web::http::client::Request, web::http::client::Response>*
    SocketContextUpgradeFactory<Args...>::create(core::socket::stream::SocketConnection* socketConnection,
                                                 web::http::client::Request* request,
                                                 web::http::client::Response* response,
                                                 Args&&... args) {
        SocketContextUpgrade* socketContext = nullptr;

        if (response->get("sec-websocket-accept") == base64::serverWebSocketKey(request->header("Sec-WebSocket-Key"))) {
            const std::string subProtocolName = response->get("sec-websocket-protocol");

            SubProtocol* subProtocol = nullptr;

            web::websocket::SubProtocolFactory<SubProtocol>* subProtocolFactory =
                SubProtocolFactorySelector::instance()->select(subProtocolName, SubProtocolFactorySelector::Role::CLIENT);

            if (subProtocolFactory != nullptr) {
                subProtocol = subProtocolFactory->createSubProtocol(std::forward(args)...);
            }

            if (subProtocol != nullptr) {
                socketContext = new SocketContextUpgrade(socketConnection, subProtocol, this, subProtocolFactory);
            }
        } else {
            this->checkRefCount();
        }

        return socketContext;
    }

    template <typename... Args>
    void SocketContextUpgradeFactory<Args...>::link() {
        static bool linked = false;

        if (!linked) {
            //            web::http::client::SocketContextUpgradeFactory<Args...>::link("websocket",
            //            websocketClientSocketContextUpgradeFactory);
            linked = true;
        }
    }

    /*
        extern "C" web::http::client::SocketContextUpgradeFactory* websocketClientSocketContextUpgradeFactory() {
            return new SocketContextUpgradeFactory();
        }
    */

} // namespace web::websocket::client
