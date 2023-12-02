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

#include "net/config/ConfigInstance.h"
#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <stdexcept>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    ConfigTls::ConfigTls(ConfigInstance* instance)
        : ConfigSection(instance, "tls", "Configuration of SSL/TLS behaviour for instance '" + instance->getInstanceName() + "'") {
        add_option(certChainOpt, //
                   "--cert-chain",
                   "Certificate chain file",
                   "filename:PEM-FILE",
                   "");

        add_option(certKeyOpt, //
                   "--cert-key",
                   "Certificate key file",
                   "filename:PEM-FILE",
                   "");

        add_option(certKeyPasswordOpt,
                   "--cert-key-password",
                   "Password for the certificate key file",
                   "password",
                   "",
                   CLI::TypeValidator<std::string>());

        add_option(caCertFileOpt, //
                   "--ca-cert-file",
                   "CA-certificate file",
                   "filename:PEM-FILE",
                   "");

        add_option(caCertDirOpt, //
                   "--ca-cert-dir",
                   "CA-certificate directory",
                   "directory:PEM-CONTAINER-DIR",
                   "");

        add_flag(useDefaultCaCertDirOpt,
                 "--ca-use-default-cert-dir{true}",
                 "Use default CA-certificate directory",
                 "bool",
                 "false",
                 CLI::IsMember({"true", "false"}));

        add_option(cipherListOpt, //
                   "--cipher-list",
                   "Cipher list (OpenSSL syntax)",
                   "cipher_list",
                   "",
                   CLI::TypeValidator<std::string>("CIPHER"));

        add_option(tlsOptionsOpt, //
                   "--tls-options",
                   "OR combined SSL/TLS options (OpenSSL values)",
                   "options",
                   0,
                   CLI::TypeValidator<ssl_option_t>());

        add_option(initTimeoutOpt, //
                   "--init-timeout",
                   "SSL/TLS initialization timeout in seconds",
                   "timeout",
                   TLS_INIT_TIMEOUT,
                   CLI::PositiveNumber);

        add_option(shutdownTimeoutOpt, //
                   "--shutdown-timeout",
                   "SSL/TLS shutdown timeout in seconds",
                   "timeout",
                   TLS_SHUTDOWN_TIMEOUT,
                   CLI::PositiveNumber);
    }

    void ConfigTls::setCertChain(const std::string& newCertChain) {
        certChainOpt //
            ->default_val(newCertChain)
            ->clear();
    }

    std::string ConfigTls::getCertChain() const {
        return certChainOpt->as<std::string>();
    }

    void ConfigTls::setCertKey(const std::string& newCertKey) {
        certKeyOpt //
            ->default_val(newCertKey)
            ->clear();
    }

    std::string ConfigTls::getCertKey() const {
        return certKeyOpt->as<std::string>();
    }

    void ConfigTls::setCertKeyPassword(const std::string& newCertKeyPassword) {
        certKeyPasswordOpt //
            ->default_val(newCertKeyPassword)
            ->clear();
    }

    std::string ConfigTls::getCertKeyPassword() const {
        return certKeyPasswordOpt->as<std::string>();
    }

    void ConfigTls::setCaCertFile(const std::string& newCaCertFile) {
        caCertFileOpt //
            ->default_val(newCaCertFile)
            ->clear();
    }

    std::string ConfigTls::getCaCertFile() const {
        return caCertFileOpt->as<std::string>();
    }

    void ConfigTls::setCaCertDir(const std::string& newCaCertDir) {
        caCertDirOpt //
            ->default_val(newCaCertDir)
            ->clear();
    }

    std::string ConfigTls::getCaCertDir() const {
        return caCertDirOpt->as<std::string>();
    }

    void ConfigTls::setUseDefaultCaCertDir(bool set) {
        useDefaultCaCertDirOpt //
            ->default_val(set ? "true" : "false")
            ->clear();
    }

    bool ConfigTls::getUseDefaultCaCertDir() const {
        return useDefaultCaCertDirOpt->as<bool>();
    }

    void ConfigTls::setCipherList(const std::string& newCipherList) {
        cipherListOpt //
            ->default_val(newCipherList)
            ->clear();
    }

    std::string ConfigTls::getCipherList() const {
        return cipherListOpt->as<std::string>();
    }

    void ConfigTls::setSslTlsOptions(ssl_option_t newSslTlsOptions) {
        tlsOptionsOpt //
            ->default_val(newSslTlsOptions)
            ->clear();
    }

    ssl_option_t ConfigTls::getSslTlsOptions() const {
        return tlsOptionsOpt->as<ssl_option_t>();
    }

    void ConfigTls::setInitTimeout(const utils::Timeval& newInitTimeout) {
        initTimeoutOpt //
            ->default_val(newInitTimeout)
            ->clear();
    }

    utils::Timeval ConfigTls::getInitTimeout() const {
        return initTimeoutOpt->as<utils::Timeval>();
    }

    void ConfigTls::setShutdownTimeout(const utils::Timeval& newShutdownTimeout) {
        shutdownTimeoutOpt //
            ->default_val(newShutdownTimeout)
            ->clear();
    }

    utils::Timeval ConfigTls::getShutdownTimeout() const {
        return shutdownTimeoutOpt //
            ->as<utils::Timeval>();
    }

} // namespace net::config
