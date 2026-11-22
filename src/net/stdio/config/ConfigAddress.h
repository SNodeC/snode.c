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

#ifndef NET_STDIO_CONFIG_CONFIGADDRESS_H
#define NET_STDIO_CONFIG_CONFIGADDRESS_H

#include "core/socket/stream/stdio/DescriptorOwnership.h"
#include "net/config/ConfigAddressBase.h" // IWYU pragma: keep
#include "net/config/ConfigAddressLocal.h"
#include "net/config/ConfigSection.h"
#include "net/stdio/SocketAddress.h"

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::stdio::config {

    using DescriptorOwnership = core::socket::stream::stdio::DescriptorOwnership;

    /// CLI-backed local binding; the owning ConfigInstance controls lifetime and event-loop use.
    class ConfigAddress
        : public net::config::ConfigSection
        , public net::config::ConfigAddressLocal<SocketAddress> {
    private:
        using SocketAddress = net::stdio::SocketAddress;
        using Super = net::config::ConfigAddressLocal<SocketAddress>;

    protected:
        /// Register only the local section, with descriptors 0/1 and TAKE defaults.
        explicit ConfigAddress(net::config::ConfigInstance* instance);

    public:
        /// Update both CLI-backed descriptor defaults; affects future address resolution.
        ConfigAddress* setSocketAddress(const SocketAddress& address);
        /// Set the default input handle without touching it.
        ConfigAddress* setInputFd(int inputFd);
        /// Read the effective configured input handle.
        int getInputFd() const;
        /// Set the default output handle without touching it.
        ConfigAddress* setOutputFd(int outputFd);
        /// Read the effective configured output handle.
        int getOutputFd() const;
        /// Set ownership for future attachments without acquiring any handle.
        ConfigAddress* setDescriptorOwnership(DescriptorOwnership ownership);
        /// Read the effective ownership policy.
        DescriptorOwnership getDescriptorOwnership() const;
        /// Enable or disable configuration of the local options together.
        void configurable(bool configurable = true) final;

    private:
        SocketAddress* init() final;

        CLI::Option* inputFdOpt;
        CLI::Option* outputFdOpt;
        CLI::Option* ownershipOpt;
    };

    /// Raw remote-address reconstruction only; never registers a CLI section or options.
    class ConfigAddressRemote : public net::config::ConfigAddressBase<SocketAddress> {
    protected:
        /// Bind to the instance lifetime without registering configuration.
        explicit ConfigAddressRemote(net::config::ConfigInstance* instance);

        /// Destroy with the owning configuration instance.
        ~ConfigAddressRemote() override;
    };

} // namespace net::stdio::config

extern template class net::config::ConfigAddressBase<net::stdio::SocketAddress>;
extern template class net::config::ConfigAddress<net::stdio::SocketAddress>;
extern template class net::config::ConfigAddressLocal<net::stdio::SocketAddress>;

#endif // NET_STDIO_CONFIG_CONFIGADDRESS_H
