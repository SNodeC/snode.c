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

#ifndef NET_RC_CONFIG_CONFIGADDRESS_H
#define NET_RC_CONFIG_CONFIGADDRESS_H

#include "net/config/ConfigAddressLocal.h"
#include "net/config/ConfigAddressRemote.h"
#include "net/config/ConfigAddressReverse.h"
#include "net/rc/SocketAddress.h"

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstdint>
#include <string>

namespace CLI {
    class Option;
} // namespace CLI

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::rc::config {

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    class ConfigAddressReverse : public ConfigAddressTypeT<net::rc::SocketAddress> {
    private:
        using Super = ConfigAddressTypeT<SocketAddress>;

    protected:
        using Super::Super;
    };

    template <template <typename SocketAddressT> typename ConfigAddressTypeT>
    class ConfigAddress : public ConfigAddressTypeT<net::rc::SocketAddress> {
    private:
        using Super = ConfigAddressTypeT<net::rc::SocketAddress>;

    protected:
        explicit ConfigAddress(net::config::ConfigInstance* instance,
                               const std::string& addressOptionName,
                               const std::string& addressOptionDescription);

    private:
        SocketAddress* init() final;

    public:
        ConfigAddress& setSocketAddress(const SocketAddress& socketAddress);

        ConfigAddress& setBtAddress(const std::string& btAddress);
        std::string getBtAddress() const;

        ConfigAddress& setChannel(uint8_t channel);
        uint8_t getChannel() const;

    protected:
        ConfigAddress& setBtAddressRequired(bool required = true);
        ConfigAddress& setChannelRequired(bool required = true);

        CLI::Option* btAddressOpt = nullptr;
        CLI::Option* channelOpt = nullptr;
    };

} // namespace net::rc::config

extern template class net::config::ConfigAddress<net::rc::SocketAddress>;
extern template class net::config::ConfigAddressLocal<net::rc::SocketAddress>;
extern template class net::config::ConfigAddressRemote<net::rc::SocketAddress>;
extern template class net::config::ConfigAddressReverse<net::rc::SocketAddress>;
extern template class net::config::ConfigAddressBase<net::rc::SocketAddress>;
extern template class net::rc::config::ConfigAddress<net::config::ConfigAddressLocal>;
extern template class net::rc::config::ConfigAddress<net::config::ConfigAddressRemote>;
extern template class net::rc::config::ConfigAddressReverse<net::config::ConfigAddressReverse>;

#endif // NET_RC_CONFIG_CONFIGADDRESS_H
