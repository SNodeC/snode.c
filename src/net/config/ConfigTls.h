/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022, 2023 Volker Christian <me@vchrist.at>
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

#ifndef NET_CONFIG_CONFIGTLS_H
#define NET_CONFIG_CONFIGTLS_H

#include "net/config/ConfigSection.h" // IWYU pragma: export

namespace net::config {
    class ConfigInstance;
}

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class Option;
} // namespace CLI

#include "core/socket/stream/tls/ssl_utils.h"   // IWYU pragma: export
#include "core/socket/stream/tls/ssl_version.h" // IWYU pragma: export
#include "utils/Timeval.h"

#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    class ConfigTls : protected ConfigSection {
    public:
        explicit ConfigTls(ConfigInstance* instance);

        utils::Timeval getInitTimeout() const;
        void setInitTimeout(const utils::Timeval& newInitTimeout);

        utils::Timeval getShutdownTimeout() const;
        void setShutdownTimeout(const utils::Timeval& newShutdownTimeout);

        std::string getCertChain() const;
        void setCertChain(const std::string& newCertChain);

        std::string getCertKey() const;
        void setCertKey(const std::string& newCertKey);

        std::string getCertKeyPassword() const;
        void setCertKeyPassword(const std::string& newCertKeyPassword);

        std::string getCaCertFile() const;
        void setCaCertFile(const std::string& newCaCertFile);

        std::string getCaCertDir() const;
        void setCaCertDir(const std::string& newCaCertDir);

        bool getUseDefaultCaCertDir() const;
        void setUseDefaultCaCertDir(bool set = true);

        std::string getCipherList() const;
        void setCipherList(const std::string& newCipherList);

        ssl_option_t getSslTlsOptions() const;
        void setSslTlsOptions(ssl_option_t newSslTlsOptions);

    private:
        CLI::Option* certChainOpt = nullptr;
        CLI::Option* certKeyOpt = nullptr;
        CLI::Option* certKeyPasswordOpt = nullptr;
        CLI::Option* caCertFileOpt = nullptr;
        CLI::Option* caCertDirOpt = nullptr;
        CLI::Option* useDefaultCaCertDirOpt = nullptr;
        CLI::Option* cipherListOpt = nullptr;
        CLI::Option* tlsOptionsOpt = nullptr;
        CLI::Option* initTimeoutOpt = nullptr;
        CLI::Option* shutdownTimeoutOpt = nullptr;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGTLS_H
