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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"
#include "utils/CLI11.hpp"

#include <any>
#include <cstdint>
#include <list>
#include <openssl/opensslv.h>
#include <stdexcept>
#include <type_traits>

// IWYU pragma: no_include <bits/utility.h>

#endif // DOXYGEN_SHOUÖD_SKIP_THIS

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
using ssl_option_t = uint64_t;
#else
using ssl_option_t = uint32_t;
#endif

namespace net::config {

    ConfigTlsServer::ConfigTlsServer() {
        if (!getName().empty()) {
            sniCertsOpt = tlsSc
                              ->add_option("--sni-cert",
                                           sniCerts,
                                           "SNI Certificates:\n"
                                           "servername = SNI name of the virtual server\n"
                                           "<key> = {\n"
                                           "          CertChain -> value = PEM file,\n"
                                           "          CertKey -> value = PEM file,\n"
                                           "          CertKeyPassword -> value = TEXT,\n"
                                           "          CaCertFile -> value = PEM file,\n"
                                           "          CaCertDir -> value = Path,\n"
                                           "          UseDefaultCaDir -> value = true|false,\n"
                                           "          SslOptions -> value = int\n"
                                           "        }")
                              ->type_name("{servername <key> value {<key> value} ...} {%% ...}");
            sniCertsOpt->take_all();

            forceSniFlg = tlsSc->add_flag("--force-sni", forceSni, "Force using of the Server Name Indication");
            forceSniFlg->type_name("[true|false]");
            forceSniFlg->default_val("false");

            tlsSc->final_callback([this](void) -> void {
                VLOG(0) << "Final Callback";
                std::list<std::string> vaultyDomainConfigs;

                for (const auto& [domain, sniMap] : sniCerts) {
                    if (!domain.empty()) {
                        for (auto& [key, value] : sniMap) {
                            if (key != "CertChain" && key != "CertKey" && key != "CertKeyPassword" && key != "CaCertFile" &&
                                key != "CaCertDir" && key != "UseDefaultCaDir" && key != "CipherList" && key != "SslOptions") {
                                LOG(ERROR) << "Configuring SNI-Cert for domain: " << domain << " failed: Wrong Key: " << key;
                                vaultyDomainConfigs.push_back(domain);
                            }
                        }
                    } else {
                        vaultyDomainConfigs.push_back(domain);
                    }
                }

                for (const std::string& domain : vaultyDomainConfigs) {
                    sniCerts.erase(domain);
                }
            });
        }
    }

    bool ConfigTlsServer::getForceSni() const {
        return forceSni;
    }

    void ConfigTlsServer::setForceSni(bool forceSni) {
        this->forceSni = forceSni;
    }

    std::map<std::string, std::map<std::string, std::any>> ConfigTlsServer::getSniCerts() const {
        return sniCerts;
    }

    void ConfigTlsServer::setSniCerts(const std::map<std::string, std::map<std::string, std::any>>& sniCert) {
        this->sniCerts = sniCert;
    }

} // namespace net::config
