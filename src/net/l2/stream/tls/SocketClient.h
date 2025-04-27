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

#ifndef NET_L2_STREAM_TLS_SOCKETCLIENT_H
#define NET_L2_STREAM_TLS_SOCKETCLIENT_H

// IWYU pragma: always_keep

#include "core/socket/stream/tls/SocketConnection.h" // IWYU pragma: export
#include "core/socket/stream/tls/SocketConnector.h"
#include "net/l2/stream/SocketClient.h"                  // IWYU pragma: export
#include "net/l2/stream/tls/config/ConfigSocketClient.h" // IWYU pragma: export

// IWYU pragma: no_include "core/socket/stream/SocketConnector.hpp"
// IWYU pragma: no_include "core/socket/stream/tls/SocketConnection.hpp"
// IWYU pragma: no_include "core/socket/stream/SocketConnection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::l2::stream::tls {

    template <typename SocketContextFactoryT, typename... Args>
    using SocketClient = net::l2::stream::SocketClient<core::socket::stream::tls::SocketConnector,
                                                       net::l2::stream::tls::config::ConfigSocketClient,
                                                       SocketContextFactoryT,
                                                       Args...>;

    template <typename SocketContextFactory, typename... SocketContextFactoryArgs>
    SocketClient<SocketContextFactory, SocketContextFactoryArgs...>
    Client(const std::string& instanceName,
              const std::function<void(typename SocketClient<SocketContextFactory, SocketContextFactoryArgs...>::Config&)>& configurator,
              SocketContextFactoryArgs&&... socketContextFactoryArgs) {
        return core::socket::stream::Client<SocketClient<SocketContextFactory, SocketContextFactoryArgs...>>(
            instanceName, configurator, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...);
    }

    template <typename SocketContextFactory,
              typename... SocketContextFactoryArgs,
              typename = std::enable_if_t<not std::is_invocable_v<std::tuple_element_t<0, std::tuple<SocketContextFactoryArgs...>>,
                                                                  typename SocketClient<SocketContextFactory>::Config&>>>
    SocketClient<SocketContextFactory, SocketContextFactoryArgs...> Client(const std::string& instanceName,
                                                                              SocketContextFactoryArgs&&... socketContextFactoryArgs) {
        return core::socket::stream::Client<SocketClient<SocketContextFactory, SocketContextFactoryArgs...>>(
            instanceName, std::forward<SocketContextFactoryArgs>(socketContextFactoryArgs)...);
    }

} // namespace net::l2::stream::tls

extern template class core::socket::Socket<net::l2::stream::tls::config::ConfigSocketClient>;
extern template class core::socket::stream::tls::SocketConnector<net::l2::phy::stream::PhysicalSocketClient,
                                                                 net::l2::stream::tls::config::ConfigSocketClient>;
extern template class core::socket::stream::tls::SocketConnection<net::l2::phy::stream::PhysicalSocketClient>;
extern template class core::socket::stream::SocketConnectionT<net::l2::phy::stream::PhysicalSocketClient,
                                                              core::socket::stream::tls::SocketReader,
                                                              core::socket::stream::tls::SocketWriter>;

#endif // NET_L2_STREAM_TLS_SOCKETCLIENT_H
