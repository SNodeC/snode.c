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

#ifndef NET_CONFIG_STREAM_CONFIGSOCKETCLIENT_H
#define NET_CONFIG_STREAM_CONFIGSOCKETCLIENT_H

#include "net/config/ConfigAddressLocal.h"         // IWYU pragma: export
#include "net/config/ConfigAddressRemote.h"        // IWYU pragma: export
#include "net/config/ConfigConnection.h"           // IWYU pragma: export
#include "net/config/ConfigPhysicalSocketClient.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config::stream {

    template <template <template <typename SocketAddress> typename ConfigAddressTypeT> typename ConfigAddressLocalT,
              template <template <typename SocketAddress> typename ConfigAddressTypeT> typename ConfigAddressRemoteT = ConfigAddressLocalT>
    class ConfigSocketClient
        : public ConfigAddressRemoteT<net::config::ConfigAddressRemote>
        , public ConfigAddressLocalT<net::config::ConfigAddressLocal>
        , public net::config::ConfigConnection
        , public net::config::ConfigPhysicalSocketClient {
    public:
        using Remote = ConfigAddressRemoteT<net::config::ConfigAddressRemote>;
        using Local = ConfigAddressLocalT<net::config::ConfigAddressLocal>;

    protected:
        explicit ConfigSocketClient(net::config::ConfigInstance* instance);
    };

} // namespace net::config::stream

#endif // NET_CONFIG_STREAM_CONFIGSOCKETCLIENT_H
