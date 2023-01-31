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

#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <stdexcept>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef DEFAULT_INITTIMEOUT
#define DEFAULT_INITTIMEOUT 10
#endif

#ifndef DEFAULT_SHUTDOWNTIMEOUT
#define DEFAULT_SHUTDOWNTIMEOUT 2
#endif

namespace net::config {

    ConfigTls::ConfigTls(ConfigInstance* instance)
        : ConfigSection(instance, "tls", "Options for SSL/TLS behaviour") {
        add_option(certChainOpt,
                   "--cert-chain",
                   "Certificate chain file",
                   "filename",
                   "",
                   CLI::detail::ExistingFileValidator().description("PEM-FILE") | CLI::IsMember({""}));
        add_option(certKeyOpt,
                   "--cert-key",
                   "Certificate key file",
                   "filename",
                   "",
                   CLI::detail::ExistingFileValidator().description("PEM-FILE") | CLI::IsMember({""}));
        add_option(certKeyPasswordOpt,
                   "--cert-key-password",
                   "Password for the certificate key file",
                   "password",
                   "",
                   CLI::TypeValidator<std::string>());
        add_option(caCertFileOpt,
                   "--ca-cert-file",
                   "CA-certificate file",
                   "filename",
                   "",
                   CLI::detail::ExistingFileValidator().description("PEM-FILE") | CLI::IsMember({""}));
        add_option(caCertDirOpt,
                   "--ca-cert-dir",
                   "CA-certificate directory",
                   "directory",
                   "",
                   CLI::detail::ExistingDirectoryValidator().description("PEM-CONTAINER") | CLI::IsMember({""}));
        add_flag(useDefaultCaCertDirOpt,
                 "--ca-use-default-cert-dir,!--ca-ignore-default-cert-dir",
                 "Use default CA-certificate directory",
                 "false",
                 CLI::TypeValidator<bool>() & !CLI::Number)
            ->type_name("bool");
        add_option(cipherListOpt, "--cipher-list", "Cipher list", "cipher_list", "");
        add_option(
            tlsOptionsOpt, "--tls-options", "OR combined SSL/TLS options", "options", 0, CLI::NonNegativeNumber | CLI::IsMember({""}));
        add_option(initTimeoutOpt,
                   "--init-timeout",
                   "SSL/TLS initialization timeout in seconds",
                   "timeout",
                   DEFAULT_INITTIMEOUT,
                   CLI::PositiveNumber | CLI::IsMember({""}));
        add_option(shutdownTimeoutOpt,
                   "--shutdown-timeout",
                   "SSL/TLS shutdown timeout in seconds",
                   "timeout",
                   DEFAULT_SHUTDOWNTIMEOUT,
                   CLI::PositiveNumber | CLI::IsMember({""}));
    }

    std::string ConfigTls::getCertChain() const {
        return certChainOpt->as<std::string>();
    }

    void ConfigTls::setCertChain(const std::string& newCertChain) {
        certChainOpt->default_val(newCertChain)->clear();
    }

    std::string ConfigTls::getCertKey() const {
        return certKeyOpt->as<std::string>();
    }

    void ConfigTls::setCertKey(const std::string& newCertKey) {
        certKeyOpt->default_val(newCertKey)->clear();
    }

    std::string ConfigTls::getCertKeyPassword() const {
        return certKeyPasswordOpt->as<std::string>();
    }

    void ConfigTls::setCertKeyPassword(const std::string& newCertKeyPassword) {
        certKeyPasswordOpt->default_val(newCertKeyPassword)->clear();
    }

    std::string ConfigTls::getCaCertFile() const {
        return caCertFileOpt->as<std::string>();
    }

    void ConfigTls::setCaCertFile(const std::string& newCaCertFile) {
        caCertFileOpt->default_val(newCaCertFile)->clear();
    }

    std::string ConfigTls::getCaCertDir() const {
        return caCertDirOpt->as<std::string>();
    }

    void ConfigTls::setCaCertDir(const std::string& newCaCertDir) {
        caCertDirOpt->default_val(newCaCertDir)->clear();
    }

    bool ConfigTls::getUseDefaultCaCertDir() const {
        return useDefaultCaCertDirOpt->as<bool>();
    }

    void ConfigTls::setUseDefaultCaCertDir(bool set) {
        useDefaultCaCertDirOpt->default_val(set ? "true" : "false")->clear();
    }

    std::string ConfigTls::getCipherList() const {
        return cipherListOpt->as<std::string>();
    }

    void ConfigTls::setCipherList(const std::string& newCipherList) {
        cipherListOpt->default_val(newCipherList)->clear();
    }

    ssl_option_t ConfigTls::getSslTlsOptions() const {
        return tlsOptionsOpt->as<ssl_option_t>();
    }

    void ConfigTls::setSslTlsOptions(ssl_option_t newSslTlsOptions) {
        tlsOptionsOpt->default_val(newSslTlsOptions)->clear();
    }

    utils::Timeval ConfigTls::getInitTimeout() const {
        return initTimeoutOpt->as<utils::Timeval>();
    }

    void ConfigTls::setInitTimeout(const utils::Timeval& newInitTimeout) {
        initTimeoutOpt->default_val(newInitTimeout)->clear();
    }

    utils::Timeval ConfigTls::getShutdownTimeout() const {
        return shutdownTimeoutOpt->as<utils::Timeval>();
    }

    void ConfigTls::setShutdownTimeout(const utils::Timeval& newShutdownTimeout) {
        shutdownTimeoutOpt->default_val(newShutdownTimeout)->clear();
    }

} // namespace net::config
