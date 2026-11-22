// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
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
        CLI::Option *inputFdOpt, *outputFdOpt, *ownershipOpt;
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
