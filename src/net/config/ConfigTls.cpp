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

#include "utils/ResetValidator.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "utils/CLI11.hpp"

#include <functional>
#include <stdexcept>

// IWYU pragma: no_include <bits/utility.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef DEFAULT_INITTIMEOUT
#define DEFAULT_INITTIMEOUT 10
#endif

#ifndef DEFAULT_SHUTDOWNTIMEOUT
#define DEFAULT_SHUTDOWNTIMEOUT 2
#endif

namespace net::config {

    struct EmptyValidator : public CLI::Validator {
        EmptyValidator();
    };

    EmptyValidator::EmptyValidator() {
        name_ = "EMPTY";
        func_ = [](const std::string& str) {
            if (!str.empty())
                return std::string("String is not empty");
            else
                return std::string();
        };
    }

    ConfigTls::ConfigTls() {
        if (!getInstanceName().empty()) {
            tlsSc = add_section("tls", "Options for SSL/TLS behaviour");

            EmptyValidator emptyValidator;

            tlsSc                                                                 //
                ->add_option("--cert-chain", certChain, "Certificate chain file") //
                ->type_name("PEM")                                                //
                ->check(CLI::ExistingFile | emptyValidator | utils::ResetValidator(tlsSc->get_option("--cert-chain")));

            tlsSc                                                           //
                ->add_option("--cert-key", certKey, "Certificate key file") //
                ->type_name("PEM")                                          //
                ->check(CLI::ExistingFile | emptyValidator | utils::ResetValidator(tlsSc->get_option("--cert-key")));

            tlsSc                                                                                             //
                ->add_option("--cert-key-password", certKeyPassword, "Password for the certificate key file") //
                ->check(utils::ResetValidator(tlsSc->get_option("--cert-key-password")));

            tlsSc                                                                 //
                ->add_option("--ca-cert-file", caCertFile, "CA-certificate file") //
                ->type_name("PEM")                                                //
                ->check(CLI::ExistingFile | emptyValidator | utils::ResetValidator(tlsSc->get_option("--ca-cert-file")));

            tlsSc                                                                    //
                ->add_option("--ca-cert-dir", caCertDir, "CA-certificate directory") //
                ->type_name("PEM_container")                                         //
                ->check(CLI::ExistingDirectory | emptyValidator | utils::ResetValidator(tlsSc->get_option("--ca-cert-dir")));

            tlsSc                                                                                                    //
                ->add_flag("--use-default-ca-cert-dir", useDefaultCaCertDir, "Use default CA-certificate directory") //
                ->default_val("false")                                                                               //
                ->check(utils::ResetValidator(tlsSc->get_option("--use-default-ca-cert-dir")));

            tlsSc                                                        //
                ->add_option("--cipher-list", cipherList, "Cipher list") //
                ->type_name("cipher_list")                               //
                ->check(utils::ResetValidator(tlsSc->get_option("--cipher-list")));

            tlsSc                                                                        //
                ->add_option("--tls-options", tlsOptions, "OR combined SSL/TLS options") //
                ->type_name("uint64_t")                                                  //
                ->check(utils::ResetValidator(tlsSc->get_option("--tls-options")));

            tlsSc                                                                                        //
                ->add_option("--init-timeout", initTimeout, "SSL/TLS initialization timeout in seconds") //
                ->default_val(DEFAULT_INITTIMEOUT)                                                       //
                ->check(utils::ResetValidator(tlsSc->get_option("--init-timeout")));

            tlsSc                                                                                          //
                ->add_option("--shutdown-timeout", shutdownTimeout, "SSL/TLS shutdown timeout in seconds") //
                ->default_val(DEFAULT_SHUTDOWNTIMEOUT)                                                     //
                ->check(utils::ResetValidator(tlsSc->get_option("--shutdown-timeout")));
        } else {
            initTimeout = DEFAULT_INITTIMEOUT;
            shutdownTimeout = DEFAULT_SHUTDOWNTIMEOUT;
        }
    }

    const std::string& ConfigTls::getCertChain() const {
        return certChain;
    }

    void ConfigTls::setCertChain(const std::string& newCertChain) {
        certChain = newCertChain;
    }

    const std::string& ConfigTls::getCertKey() const {
        return certKey;
    }

    void ConfigTls::setCertKey(const std::string& newCertKey) {
        certKey = newCertKey;
    }

    const std::string& ConfigTls::getCertKeyPassword() const {
        return certKeyPassword;
    }

    void ConfigTls::setCertKeyPassword(const std::string& newCertKeyPassword) {
        certKeyPassword = newCertKeyPassword;
    }

    const std::string& ConfigTls::getCaCertFile() const {
        return caCertFile;
    }

    void ConfigTls::setCaCertFile(const std::string& newCaCertFile) {
        caCertFile = newCaCertFile;
    }

    const std::string& ConfigTls::getCaCertDir() const {
        return caCertDir;
    }

    void ConfigTls::setCaCertDir(const std::string& newCaCertDir) {
        caCertDir = newCaCertDir;
    }

    bool ConfigTls::getUseDefaultCaCertDir() const {
        return useDefaultCaCertDir;
    }

    void ConfigTls::setUseDefaultCaCertDir() {
        useDefaultCaCertDir = true;
    }

    const std::string& ConfigTls::getCipherList() const {
        return cipherList;
    }

    void ConfigTls::setCipherList(const std::string& newCipherList) {
        cipherList = newCipherList;
    }

    const uint64_t& ConfigTls::getSslTlsOptions() const {
        return tlsOptions;
    }

    void ConfigTls::setSslTlsOptions(uint64_t newSslTlsOptions) {
        tlsOptions = newSslTlsOptions;
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
