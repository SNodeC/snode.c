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

#include "net/config/ConfigTlsServer.h"

#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>
#include <memory>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace net::config {

    ConfigTlsServer::ConfigTlsServer(ConfigInstance* instance)
        : ConfigTls(instance) {
        sniCertsOpt = section //
                          ->addOption("--sni-cert",
                                       sniCerts,
                                       "Server Name Indication (SNI) Certificates:\n"
                                       "sni = SNI of the virtual server\n"
                                       "<key> = {\n"
                                       "  Cert -> value:PEM-FILE                  ['']\n"
                                       "  CertKey -> value:PEM-FILE               ['']\n"
                                       "  CertKeyPassword -> value:TEXT           ['']\n"
                                       "  CaCert -> value:PEM-FILE                ['']\n"
                                       "  CaCertDir -> value:PEM-CONTAINER-DIR    ['']\n"
                                       "  CaCertUseDefaultDir -> value:BOOLEAN    [false]\n"
                                       "  CipherList -> value:CIPHER              ['']\n"
                                       "  SslOptions -> value:UINT                [0]\n"
                                       "}") //
                          ->type_name("sni <key> value [<key> value] ... [%% sni <key> value [<key> value] ...]")
                          ->default_str("'' '' ''");
        if (sniCertsOpt->get_configurable()) {
            sniCertsOpt->group(section->get_formatter()->get_label("Persistent Options"));
        }

        sniCertsOpt->default_function([&sniCerts = this->sniCerts]() -> std::string {
            std::string defaultValue;

            for (const auto& [domain, sniCertConf] : sniCerts) {
                defaultValue += (!defaultValue.empty() ? "%% " : "") + domain + " ";

                for (const auto& [key, value] : sniCertConf) {
                    defaultValue += key + " ";

                    if (key == "Cert") {
                        defaultValue += std::get<std::string>(value) + " ";
                    } else if (key == "CertKey") {
                        defaultValue += std::get<std::string>(value) + " ";
                    } else if (key == "CertKeyPassword") {
                        defaultValue += std::get<std::string>(value) + " ";
                    } else if (key == "CaCert") {
                        defaultValue += std::get<std::string>(value) + " ";
                    } else if (key == "CaCertDir") {
                        defaultValue += std::get<std::string>(value) + " ";
                    } else if (key == "CaCertUseDefaultDir") {
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

        forceSniOpt = addFlag( //
            "--force-sni{true}",
            "Force using of the Server Name Indication",
            "bool",
            "false",
            CLI::IsMember({"true", "false"}));

        section->final_callback([this]() -> void {
            for (auto& [domain, sniMap] : sniCerts) {
                if (domain.empty()) {
                    sniCertsOpt //
                        ->clear();
                    sniMap.clear();
                    break;
                }
                for (auto& [key, value] : sniMap) {
                    if (key != "Cert" && key != "CertKey" && key != "CertKeyPassword" && key != "CaCert" && key != "CaCertDir" &&
                        key != "CaCertUseDefaultDir" && key != "CipherList" && key != "SslOptions") {
                        throw CLI::ConversionError("'" + key + "' of option '--" + section->get_parent()->get_name() + "." +
                                                       section->get_name() + ".sni-cert'",
                                                   "<key>");
                    }
                }
            }
        });
    }

    ConfigTlsServer& ConfigTlsServer::setForceSni(bool forceSni) {
        forceSniOpt //
            ->default_val(forceSni ? "true" : "false")
            ->clear();

        return *this;
    }

    bool ConfigTlsServer::getForceSni() const {
        return forceSniOpt->as<bool>();
    }

    ConfigTlsServer& ConfigTlsServer::addSniCerts(
        const std::map<std::string, std::map<std::string, std::variant<std::string, bool, ssl_option_t>>>& sniCerts) {
        this->sniCerts.insert(sniCerts.begin(), sniCerts.end());

        sniCertsOpt->capture_default_str();

        return *this;
    }

    ConfigTlsServer& ConfigTlsServer::addSniCert(const std::string& domain,
                                                 const std::map<std::string, std::variant<std::string, bool, ssl_option_t>>& sniCert) {
        this->sniCerts[domain] = sniCert;

        sniCertsOpt->capture_default_str();

        return *this;
    }

    std::map<std::string, std::map<std::string, std::variant<std::string, bool, ssl_option_t>>>& ConfigTlsServer::getSniCerts() {
        return sniCerts;
    }

} // namespace net::config
