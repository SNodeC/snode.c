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

#include "net/config/ConfigTls.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#include <stdexcept>

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

    const std::string& ConfigTls::getCertChainFile() const {
        return certChainFile;
    }

    void ConfigTls::setCertChainFile(const std::string& newCertChainFile) {
        certChainFile = newCertChainFile;
    }

    const std::string& ConfigTls::getCertKeyFile() const {
        return certKeyFile;
    }

    void ConfigTls::setCertKeyFile(const std::string& newCertKeyFile) {
        certKeyFile = newCertKeyFile;
    }

    const std::string& ConfigTls::getCertKeyPassword() const {
        return certKeyPassword;
    }

    void ConfigTls::setCertKeyPassword(const std::string& newCertKeyPassword) {
        certKeyPassword = newCertKeyPassword;
    }

    const std::string& ConfigTls::getCaFile() const {
        return caFile;
    }

    void ConfigTls::setCaFile(const std::string& newCaFile) {
        caFile = newCaFile;
    }

    const std::string& ConfigTls::getCaDir() const {
        return caDir;
    }

    void ConfigTls::setCaDir(const std::string& newCaDir) {
        caDir = newCaDir;
    }

    bool ConfigTls::getUseDefaultCaDir() const {
        return useDefaultCaDir;
    }

    void ConfigTls::setUseDefaultCaDir() {
        useDefaultCaDir = true;
    }

    const std::string& ConfigTls::getSni() const {
        return sni;
    }

    void ConfigTls::setSni(const std::string& newSni) {
        sni = newSni;
    }

    bool ConfigTls::getForceSni() const {
        return forceSni;
    }

    void ConfigTls::setForceSni() {
        forceSni = true;
    }

    const std::string& ConfigTls::getCipherList() const {
        return cipherList;
    }

    void ConfigTls::setCipherList(const std::string& newCipherList) {
        cipherList = newCipherList;
    }

    const uint64_t& ConfigTls::getSslTlsOptions() const {
        return sslTlsOptions;
    }

    void ConfigTls::setSslTlsOptions(uint64_t newSslTlsOptions) {
        sslTlsOptions = newSslTlsOptions;
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

    const utils::Timeval& ConfigTls::getInitTimeout() const {
        return initTimeout;
    }

    void ConfigTls::setInitTimeout(const utils::Timeval& newInitTimeout) {
        initTimeout = newInitTimeout;
    }

    const utils::Timeval& ConfigTls::getShutdownTimeout() const {
        return shutdownTimeout;
    }

    void ConfigTls::setShutdownTimeout(const utils::Timeval& newShutdownTimeout) {
        shutdownTimeout = newShutdownTimeout;
    }

} // namespace net::config
