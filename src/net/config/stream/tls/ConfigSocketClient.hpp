/*
 * SNode.C - a slim toolkit for network communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024
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

#include "net/config/stream/tls/ConfigSocketClient.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config::stream::tls {

    template <typename ConfigSocketClientBase>
    ConfigSocketClient<ConfigSocketClientBase>::ConfigSocketClient(const std::string& name)
        : net::config::ConfigInstance(name, net::config::ConfigInstance::Role::CLIENT)
        , ConfigSocketClientBase(this)
        , net::config::ConfigTlsClient(this) {
    }

    template <typename ConfigSocketClientBase>
    ConfigSocketClient<ConfigSocketClientBase>::~ConfigSocketClient() {
        if (sslCtx != nullptr) {
            core::socket::stream::tls::ssl_ctx_free(sslCtx);
        }
    }

    template <typename ConfigSocketClientBase>
    SSL_CTX* ConfigSocketClient<ConfigSocketClientBase>::getSslCtx() {
        if (sslCtx == nullptr) {
            core::socket::stream::tls::SslConfig sslConfig(false);

            sslConfig.instanceName = getInstanceName();

            sslConfig.cert = getCert();
            sslConfig.certKey = getCertKey();
            sslConfig.password = getCertKeyPassword();
            sslConfig.caCert = getCaCert();
            sslConfig.caCertDir = getCaCertDir();
            sslConfig.cipherList = getCipherList();
            sslConfig.sslOptions = getSslOptions();
            sslConfig.caCertUseDefaultDir = getCaCertUseDefaultDir();
            sslConfig.caCertAcceptUnknown = getCaCertAcceptUnknown();

            sslCtx = core::socket::stream::tls::ssl_ctx_new(sslConfig);
        }

        return sslCtx;
    }

} // namespace net::config::stream::tls
