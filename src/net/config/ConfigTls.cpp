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

#include "net/config/ConfigTls.h"

#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    ConfigTls::ConfigTls(ConfigInstance* instance)
        : ConfigSection(instance, "tls", "Configuration of SSL/TLS behavior") {
        certOpt = addOption( //
            "--cert",
            "Certificate chain file",
            "filename:PEM-FILE",
            "");

        certKeyOpt = addOption( //
            "--cert-key",
            "Certificate key file",
            "filename:PEM-FILE",
            "");

        certKeyPasswordOpt = addOption( //
            "--cert-key-password",
            "Password for the certificate key file",
            "password",
            "",
            CLI::TypeValidator<std::string>());

        caCertOpt = addOption( //
            "--ca-cert",
            "CA-certificate file",
            "filename:PEM-FILE",
            "");

        caCertDirOpt = addOption( //
            "--ca-cert-dir",
            "CA-certificate directory",
            "directory:PEM-CONTAINER-DIR",
            "");

        caCertUseDefaultDirOpt = addFlag( //
            "--ca-cert-use-default-dir{true}",
            "Use default CA-certificate directory",
            "bool",
            "false",
            CLI::IsMember({"true", "false"}));

        cipherListOpt = addOption( //
            "--cipher-list",
            "Cipher list (OpenSSL syntax)",
            "cipher_list",
            "",
            CLI::TypeValidator<std::string>("CIPHER"));

        sslOptionsOpt = addOption( //
            "--ssl-options",
            "OR combined SSL/TLS options (OpenSSL values)",
            "options",
            0,
            CLI::TypeValidator<ssl_option_t>());

        initTimeoutOpt = addOption( //
            "--init-timeout",
            "SSL/TLS initialization timeout in seconds",
            "timeout",
            TLS_INIT_TIMEOUT,
            CLI::PositiveNumber);

        shutdownTimeoutOpt = addOption( //
            "--shutdown-timeout",
            "SSL/TLS shutdown timeout in seconds",
            "timeout",
            TLS_SHUTDOWN_TIMEOUT,
            CLI::PositiveNumber);

        noCloseNotifyIsEOFOpt = addFlag( //
            "--no-close-notify-is-eof{true}",
            "Do not interpret a SSL/TLS close_notify alert as EOF",
            "bool",
            "false",
            CLI::IsMember({"true", "false"}));
    }

    ConfigTls& ConfigTls::setCert(const std::string& cert) {
        certOpt //
            ->default_val(cert)
            ->clear();

        return *this;
    }

    std::string ConfigTls::getCert() const {
        return certOpt->as<std::string>();
    }

    ConfigTls& ConfigTls::setCertKey(const std::string& certKey) {
        certKeyOpt //
            ->default_val(certKey)
            ->clear();

        return *this;
    }

    std::string ConfigTls::getCertKey() const {
        return certKeyOpt->as<std::string>();
    }

    ConfigTls& ConfigTls::setCertKeyPassword(const std::string& certKeyPassword) {
        certKeyPasswordOpt //
            ->default_val(certKeyPassword)
            ->clear();

        return *this;
    }

    std::string ConfigTls::getCertKeyPassword() const {
        return certKeyPasswordOpt->as<std::string>();
    }

    ConfigTls& ConfigTls::setCaCert(const std::string& caCert) {
        caCertOpt //
            ->default_val(caCert)
            ->clear();

        return *this;
    }

    std::string ConfigTls::getCaCert() const {
        return caCertOpt->as<std::string>();
    }

    ConfigTls& ConfigTls::setCaCertDir(const std::string& caCertDir) {
        caCertDirOpt //
            ->default_val(caCertDir)
            ->clear();

        return *this;
    }

    std::string ConfigTls::getCaCertDir() const {
        return caCertDirOpt->as<std::string>();
    }

    ConfigTls& ConfigTls::setCaCertUseDefaultDir(bool set) {
        caCertUseDefaultDirOpt //
            ->default_val(set ? "true" : "false")
            ->clear();

        return *this;
    }

    bool ConfigTls::getCaCertUseDefaultDir() const {
        return caCertUseDefaultDirOpt->as<bool>();
    }

    ConfigTls& ConfigTls::setCipherList(const std::string& cipherList) {
        cipherListOpt //
            ->default_val(cipherList)
            ->clear();

        return *this;
    }

    std::string ConfigTls::getCipherList() const {
        return cipherListOpt->as<std::string>();
    }

    ConfigTls& ConfigTls::setSslOptions(ssl_option_t sslOptions) {
        sslOptionsOpt //
            ->default_val(sslOptions)
            ->clear();

        return *this;
    }

    ssl_option_t ConfigTls::getSslOptions() const {
        return sslOptionsOpt->as<ssl_option_t>();
    }

    ConfigTls& ConfigTls::setNoCloseNotifyIsEOF(bool closeNotifyIsEOF) {
        noCloseNotifyIsEOFOpt //
            ->default_val(closeNotifyIsEOF ? "true" : "false")
            ->clear();
        return *this;
    }

    bool ConfigTls::getNoCloseNotifyIsEOF() const {
        return noCloseNotifyIsEOFOpt->as<bool>();
    }

    ConfigTls& ConfigTls::setInitTimeout(const utils::Timeval& newInitTimeout) {
        initTimeoutOpt //
            ->default_val(newInitTimeout)
            ->clear();

        return *this;
    }

    utils::Timeval ConfigTls::getInitTimeout() const {
        return initTimeoutOpt->as<utils::Timeval>();
    }

    ConfigTls& ConfigTls::setShutdownTimeout(const utils::Timeval& newShutdownTimeout) {
        shutdownTimeoutOpt //
            ->default_val(newShutdownTimeout)
            ->clear();

        return *this;
    }

    utils::Timeval ConfigTls::getShutdownTimeout() const {
        return shutdownTimeoutOpt //
            ->as<utils::Timeval>();
    }

} // namespace net::config
