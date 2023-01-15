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

#include "net/config/ConfigInstance.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
} // namespace CLI

#include "utils/Timeval.h"

#include <cstdint>
#include <map>
#include <openssl/opensslv.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
using ssl_option_t = uint64_t;
#else
using ssl_option_t = uint32_t;
#endif

namespace net::config {

    class ConfigTls : virtual public ConfigInstance {
    public:
        ConfigTls();

        const utils::Timeval& getInitTimeout() const;
        void setInitTimeout(const utils::Timeval& newInitTimeout);

        const utils::Timeval& getShutdownTimeout() const;
        void setShutdownTimeout(const utils::Timeval& newShutdownTimeout);

        const std::string& getCertChainFile() const;
        void setCertChainFile(const std::string& newCertChainFile);

        const std::string& getCertKeyFile() const;
        void setCertKeyFile(const std::string& newCertKeyFile);

        const std::string& getCertKeyPassword() const;
        void setCertKeyPassword(const std::string& newCertKeyPassword);

        const std::string& getCaFile() const;
        void setCaFile(const std::string& newCaFile);

        const std::string& getCaDir() const;
        void setCaDir(const std::string& newCaDir);

        bool getUseDefaultCaDir() const;
        void setUseDefaultCaDir();

        bool getForceSni() const;
        void setForceSni();

        const std::string& getCipherList() const;
        void setCipherList(const std::string& newCipherList);

        const uint64_t& getSslTlsOptions() const;
        void setSslTlsOptions(uint64_t newSslTlsOptions);

    protected:
        CLI::App* tlsSc = nullptr;

    private:
        CLI::Option* certChainFileOpt;
        CLI::Option* certKeyFileOpt;
        CLI::Option* certKeyPasswordOpt;
        CLI::Option* caFileOpt;
        CLI::Option* caDirOpt;
        CLI::Option* useDefaultCaDirFlg;
        CLI::Option* cipherListOpt;
        CLI::Option* sslTlsOptionsOpt;

        CLI::Option* initTimeoutOpt = nullptr;
        CLI::Option* shutdownTimeoutOpt = nullptr;

        std::string certChainFile;
        std::string certKeyFile;
        std::string certKeyPassword;
        std::string caFile;
        std::string caDir;
        bool useDefaultCaDir = false;
        std::string cipherList;
        ssl_option_t sslTlsOptions = 0;

        utils::Timeval initTimeout;
        utils::Timeval shutdownTimeout;

        std::map<std::string, std::map<std::string, std::string>> sniCerts;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGTLS_H
