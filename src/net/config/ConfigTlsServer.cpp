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

#include "net/config/ConfigTlsServer.h"

#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <list>
#include <stdexcept>
#include <type_traits>
#include <utility>

// IWYU pragma: no_include <bits/utility.h>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

namespace net::config {

    ConfigTlsServer::ConfigTlsServer(ConfigInstance* instance)
        : ConfigTls(instance) {
        sniCertsOpt = section //
                          ->add_option("--sni-cert",
                                       sniCerts,
                                       "Server Name Indication (SNI) Certificates:\n"
                                       "sni = SNI of the virtual server\n"
                                       "<key> = {\n"
                                       "          \"CertChain\" -> value:PEM-FILE,\n"
                                       "          \"CertKey\" -> value:PEM-FILE,\n"
                                       "          \"CertKeyPassword\" -> value:TEXT,\n"
                                       "          \"CaCertFile\" -> value:PEM-FILE,\n"
                                       "          \"CaCertDir\" -> value:PEM-CONTAINER:DIR,\n"
                                       "          \"UseDefaultCaDir\" -> value:BOOLEAN [false],\n"
                                       "          \"SslOptions\" -> value:POSITIV\n"
                                       "        }") //
                          ->type_name("sni <key> value {<key> value} ... {%% sni <key> value {<key> value} ...}")
                          ->default_val("\"\" \"\" \"\" \"\"");

        add_flag(forceSniOpt,
                 "--sni-required,!--sni-optional",
                 "Force using of the Server Name Indication",
                 "bool",
                 "false",
                 CLI::TypeValidator<bool>() & !CLI::Number); // cppcheck-suppress clarifyCondition

        section->final_callback([this](void) -> void {
            std::list<std::string> vaultyDomainConfigs;
            for (const auto& [domain, sniMap] : sniCerts) {
                if (sniMap.begin()->first.empty()) {
                    sniCertsOpt //
                        ->clear();
                    break;
                } else {
                    for (auto& [key, value] : sniMap) {
                        if (key != "CertChain" && key != "CertKey" && key != "CertKeyPassword" && key != "CaCertFile" &&
                            key != "CaCertDir" && key != "UseDefaultCaDir" && key != "CipherList" && key != "SslOptions") {
                            vaultyDomainConfigs.push_back(key);
                            throw CLI::ConversionError("'" + key + "' of option '--" + section->get_parent()->get_name() + "." +
                                                           section->get_name() + ".sni-cert'",
                                                       "<key>");
                        }
                    }
                }
            }

            for (const std::string& domain : vaultyDomainConfigs) {
                sniCerts.erase(domain);
            }
        });
    }

    bool ConfigTlsServer::getForceSni() const {
        return forceSniOpt //
            ->as<bool>();
    }

    void ConfigTlsServer::setForceSni(bool forceSni) {
        forceSniOpt //
            ->default_val(forceSni ? "true" : "false")
            ->clear();
    }

    std::map<std::string, std::map<std::string, std::variant<std::string, bool, ssl_option_t>>>& ConfigTlsServer::getSniCerts() {
        return sniCerts;
    }

    void ConfigTlsServer::setSniCerts(
        const std::map<std::string, std::map<std::string, std::variant<std::string, bool, ssl_option_t>>>& sniCert) {
        this->sniCerts = sniCert;
    }

} // namespace net::config
