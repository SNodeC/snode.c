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

#ifndef NET_CONFIG_CONFIGTLS_H
#define NET_CONFIG_CONFIGTLS_H

#include "net/config/ConfigSection.h"

namespace net::config {
    class ConfigInstance;
}

#include "core/socket/stream/tls/ssl_utils.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class Option;
} // namespace CLI

#include "utils/Timeval.h"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    class ConfigTls : protected ConfigSection {
    protected:
        explicit ConfigTls(ConfigInstance* instance);

    public:
        ConfigTls& setInitTimeout(const utils::Timeval& newInitTimeout);
        utils::Timeval getInitTimeout() const;

        ConfigTls& setShutdownTimeout(const utils::Timeval& newShutdownTimeout);
        utils::Timeval getShutdownTimeout() const;

        ConfigTls& setCert(const std::string& cert);
        std::string getCert() const;

        ConfigTls& setCertKey(const std::string& certKey);
        std::string getCertKey() const;

        ConfigTls& setCertKeyPassword(const std::string& certKeyPassword);
        std::string getCertKeyPassword() const;

        ConfigTls& setCaCert(const std::string& caCert);
        std::string getCaCert() const;

        ConfigTls& setCaCertDir(const std::string& caCertDir);
        std::string getCaCertDir() const;

        ConfigTls& setCaCertUseDefaultDir(bool set = true);
        bool getCaCertUseDefaultDir() const;

        ConfigTls& setCipherList(const std::string& cipherList);
        std::string getCipherList() const;

        ConfigTls& setSslOptions(ssl_option_t sslOptions);
        ssl_option_t getSslOptions() const;

    private:
        CLI::Option* certOpt = nullptr;
        CLI::Option* certKeyOpt = nullptr;
        CLI::Option* certKeyPasswordOpt = nullptr;
        CLI::Option* caCertOpt = nullptr;
        CLI::Option* caCertDirOpt = nullptr;
        CLI::Option* caCertUseDefaultDirOpt = nullptr;
        CLI::Option* cipherListOpt = nullptr;
        CLI::Option* sslOptionsOpt = nullptr;
        CLI::Option* initTimeoutOpt = nullptr;
        CLI::Option* shutdownTimeoutOpt = nullptr;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGTLS_H
