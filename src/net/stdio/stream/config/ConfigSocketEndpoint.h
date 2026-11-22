// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#ifndef NET_STDIO_STREAM_CONFIG_CONFIGSOCKETENDPOINT_H
#define NET_STDIO_STREAM_CONFIG_CONFIGSOCKETENDPOINT_H

#include "net/config/ConfigConnection.h"
#include "net/config/ConfigInstance.h"
#include "net/stdio/config/ConfigAddress.h"

#include <string>
namespace net::stdio::stream::config {

    /// Original instance/connection policy and local descriptor options; Remote is non-CLI.
    class ConfigSocketEndpoint
        : public net::config::ConfigInstance
        , public net::config::ConfigConnection
        , public net::stdio::config::ConfigAddress
        , public net::stdio::config::ConfigAddressRemote {
    public:
        using Connection = net::config::ConfigConnection;
        using Local = net::stdio::config::ConfigAddress;
        using Remote = net::stdio::config::ConfigAddressRemote;
        /// Register an endpoint instance before init; descriptor ownership begins only on activation.
        explicit ConfigSocketEndpoint(const std::string& name);
        /// Destroy after all shared wrappers/connections release their configuration.
        ~ConfigSocketEndpoint() override;
    };

} // namespace net::stdio::stream::config
#endif // NET_STDIO_STREAM_CONFIG_CONFIGSOCKETENDPOINT_H
