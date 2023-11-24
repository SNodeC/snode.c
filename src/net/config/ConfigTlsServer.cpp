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

#include <memory>
#include <stdexcept>
#include <utility>

// IWYU pragma: no_include <bits/utility.h>

#endif // DOXYGEN_SHOULD_SKIP_THIS

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
                                       "          \"SslOptions\" -> value:UINT\n"
                                       "        }") //
                          ->type_name("sni <key> value [<key> value] ... [%% sni <key> value [<key> value] ...]");
        if (sniCertsOpt->get_configurable()) {
            sniCertsOpt->group(section->get_formatter()->get_label("Persistent Options"));
        }

        sniCertsOpt->default_function([&sniCerts = this->sniCerts]() -> std::string {
            std::string defaultValue;

            for (const auto& [domain, sniCertConf] : sniCerts) {
                defaultValue += (!defaultValue.empty() ? "%% " : "") + domain + " ";

                for (const auto& [key, value] : sniCertConf) {
                    defaultValue += key + " ";

                    if (key == "CertChain") {
                        defaultValue += std::get<std::string>(value) + " ";
                    } else if (key == "CertKey") {
                        defaultValue += std::get<std::string>(value) + " ";
                    } else if (key == "CertKeyPassword") {
                        defaultValue += std::get<std::string>(value) + " ";
                    } else if (key == "CaCertFile") {
                        defaultValue += std::get<std::string>(value) + " ";
                    } else if (key == "CaCertDir") {
                        defaultValue += std::get<std::string>(value) + " ";
                    } else if (key == "UseDefaultCaDir") {
                        defaultValue += std::get<bool>(value) ? "true " : "false ";
                    } else if (key == "CipherList") {
                        defaultValue += std::get<std::string>(value) + " ";
                    } else if (key == "SslOptions") {
                        defaultValue += std::to_string(std::get<ssl_option_t>(value)) + " ";
                    }
                }
            }

            defaultValue.pop_back();

            return defaultValue;
        });

        add_flag(
            forceSniOpt, "--force-sni", "Force using of the Server Name Indication", "bool", "false", CLI::IsMember({"true", "false"}));

        section->final_callback([this]() -> void {
            for (auto& [domain, sniMap] : sniCerts) {
                if (sniMap.begin()->first.empty()) {
                    sniCertsOpt //
                        ->clear();
                    break;
                }
                for (auto& [key, value] : sniMap) {
                    if (key != "CertChain" && key != "CertKey" && key != "CertKeyPassword" && key != "CaCertFile" && key != "CaCertDir" &&
                        key != "UseDefaultCaDir" && key != "CipherList" && key != "SslOptions") {
                        throw CLI::ConversionError("'" + key + "' of option '--" + section->get_parent()->get_name() + "." +
                                                       section->get_name() + ".sni-cert'",
                                                   "<key>");
                    }
                }
            }
        });
    }

    void ConfigTlsServer::setForceSni(bool forceSni) {
        forceSniOpt //
            ->default_val(forceSni ? "true" : "false")
            ->clear();
    }

    bool ConfigTlsServer::getForceSni() const {
        return forceSniOpt->as<bool>();
    }

    void ConfigTlsServer::addSniCerts(
        const std::map<std::string, std::map<std::string, std::variant<std::string, bool, ssl_option_t>>>& sniCerts) {
        this->sniCerts.insert(sniCerts.begin(), sniCerts.end());

        sniCertsOpt->capture_default_str();
    }

    void ConfigTlsServer::addSniCert(const std::string& domain,
                                     const std::map<std::string, std::variant<std::string, bool, ssl_option_t>>& sniCert) {
        this->sniCerts[domain] = sniCert;

        sniCertsOpt->capture_default_str();
    }

    std::map<std::string, std::map<std::string, std::variant<std::string, bool, ssl_option_t>>>& ConfigTlsServer::getSniCerts() {
        return sniCerts;
    }

} // namespace net::config
