/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020, 2021, 2022 Volker Christian <me@vchrist.at>
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

#include "net/config/ConfigTls.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef DEFAULT_INITTIMEOUT
#define DEFAULT_INITTIMEOUT 10
#endif

#ifndef DEFAULT_SHUTDOWNTIMEOUT
#define DEFAULT_SHUTDOWNTIMEOUT 2
#endif

namespace net::config {

    ConfigTls::ConfigTls() {
        if (!getName().empty()) {
            tlsSc = add_subcommand("tls", "Options for SSL/TLS behaviour");
            tlsSc->group("Subcommands");

            certChainFileOpt = tlsSc->add_option("--cert-chain", certChainFile, "Certificate chain file");
            certChainFileOpt->type_name("[PEM file]");
            certKeyFileOpt = tlsSc->add_option("--cert-key", certKeyFile, "Certificate key file");
            certKeyFileOpt->type_name("[PEM file]");
            certKeyPasswordOpt = tlsSc->add_option("--cert-key-password", certKeyPassword, "Password for the certificate key file");
            certKeyPasswordOpt->type_name("[Password]");
            caFileOpt = tlsSc->add_option("--ca-cert-file", caFile, "CA-certificate file");
            caFileOpt->type_name("[PEM file]");
            caDirOpt = tlsSc->add_option("--ca-cert-dir", caDir, "CA-certificate directory");
            caDirOpt->type_name("[Path]");
            useDefaultCaDirFlg =
                tlsSc->add_flag("--use-default-ca-cert-dir{true}", useDefaultCaDir, "Use default CA-certificate directory");
            sniOpt = tlsSc->add_option("--sni", sni, "Server Name Indication");
            sniOpt->type_name("[Hostname or IP]");
            forceSniFlg = tlsSc->add_flag("--force-sni{true}", forceSni, "Force using of the Server Name Indication");
            cipherListOpt = tlsSc->add_option("--cipher-list", cipherList, "Cipher list");
            cipherListOpt->type_name("[Cipher List]");
            sslTlsOptionsOpt = tlsSc->add_option("--tls-options", sslTlsOptions, "OR combined SSL/TLS options");
            sslTlsOptionsOpt->type_name("[int]");

            initTimeoutOpt = tlsSc->add_option("--init-timeout", initTimeout, "SSL/TLS initialization timeout");
            initTimeoutOpt->type_name("[sec]");
            initTimeoutOpt->default_val(DEFAULT_INITTIMEOUT);

            shutdownTimeoutOpt = tlsSc->add_option("--shutdown-timeout", shutdownTimeout, "SSL/TLS shutdown timeout");
            shutdownTimeoutOpt->type_name("[sec]");
            shutdownTimeoutOpt->default_val(DEFAULT_SHUTDOWNTIMEOUT);
        } else {
            initTimeout = DEFAULT_INITTIMEOUT;
            shutdownTimeout = DEFAULT_SHUTDOWNTIMEOUT;
        }
    }

    utils::Timeval ConfigTls::getInitTimeout() const {
        utils::Timeval initTimeout = this->initTimeout;

        if (initTimeoutSet >= 0 && initTimeoutOpt != nullptr && initTimeoutOpt->count() == 0) {
            initTimeout = initTimeoutSet;
        }

        return initTimeout;
    }

    utils::Timeval ConfigTls::getShutdownTimeout() const {
        utils::Timeval shutdownTimeout = this->shutdownTimeout;

        if (shutdownTimeoutSet >= 0 && shutdownTimeoutOpt != nullptr && shutdownTimeoutOpt->count() == 0) {
            shutdownTimeout = shutdownTimeoutSet;
        }

        return shutdownTimeout;
    }

    void ConfigTls::setInitTimeout(const utils::Timeval& newInitTimeoutSet) {
        initTimeoutSet = newInitTimeoutSet;
    }

    void ConfigTls::setShutdownTimeout(const utils::Timeval& newShutdownTimeoutSet) {
        shutdownTimeoutSet = newShutdownTimeoutSet;
    }

    const std::string& ConfigTls::getCertChainFile() const {
        return certChainFile;
    }

    const std::string& ConfigTls::getCertKeyFile() const {
        return certKeyFile;
    }

    const std::string& ConfigTls::getCertKeyPassword() const {
        return certKeyPassword;
    }

    const std::string& ConfigTls::getCaFile() const {
        return caFile;
    }

    const std::string& ConfigTls::getCaDir() const {
        return caDir;
    }

    bool ConfigTls::getUseDefaultCaDir() const {
        return useDefaultCaDir;
    }

    const std::string& ConfigTls::getSni() const {
        return sni;
    }

    bool ConfigTls::getForceSni() const {
        return forceSni;
    }

    const std::string& ConfigTls::getCipherList() const {
        return cipherList;
    }

    const uint64_t& ConfigTls::getSslTlsOptions() const {
        return sslTlsOptions;
    }

    void ConfigTls::disableSni() {
        if (tlsSc != nullptr) {
            tlsSc->remove_option(sniOpt);
        }
    }

    void ConfigTls::disableForceSni() {
        if (tlsSc != nullptr) {
            tlsSc->remove_option(forceSniFlg);
        }
    }

} // namespace net::config
