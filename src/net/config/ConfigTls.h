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

#include "net/config/ConfigBase.h" // IWYU pragma: export

#ifndef DOXYGEN_SHOULD_SKIP_THIS

namespace CLI {
    class App;
    class Option;
} // namespace CLI

#include "utils/Timeval.h"

#include <cstdint>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    class ConfigTls : virtual public ConfigBase {
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

        const std::string& getSni() const;
        void setSni(const std::string& newSni);

        bool getForceSni() const;
        void setForceSni();

        const std::string& getCipherList() const;
        void setCipherList(const std::string& newCipherList);

        const uint64_t& getSslTlsOptions() const;
        void setSslTlsOptions(uint64_t newSslTlsOptions);

        void disableSni();
        void disableForceSni();

    private:
        CLI::App* tlsSc = nullptr;
        CLI::Option* certChainFileOpt;
        CLI::Option* certKeyFileOpt;
        CLI::Option* certKeyPasswordOpt;
        CLI::Option* caFileOpt;
        CLI::Option* caDirOpt;
        CLI::Option* useDefaultCaDirFlg;
        CLI::Option* sniOpt;
        CLI::Option* forceSniFlg;
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
        std::string sni;
        bool forceSni = false;
        std::string cipherList;
        uint64_t sslTlsOptions = 0;

        utils::Timeval initTimeout;
        utils::Timeval shutdownTimeout;
    };

} // namespace net::config

#endif // NET_CONFIG_CONFIGTLS_H
