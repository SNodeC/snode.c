#include "utils/CLI11.hpp"

#include <any>
#include <iostream>
#include <map>
//#include <stdexcept>
#include <string>
//#include <utility>

#include <cstdint>
#include <openssl/opensslv.h>
#include <stdexcept>
#include <type_traits>

// IWYU pragma: no_include <bits/utility.h>

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
using ssl_option_t = uint64_t;
#else
using ssl_option_t = uint32_t;
#endif

int main(int argc, char** argv) {
    CLI::App app("data output specification");
    app.set_help_all_flag("--help-all", "Expand all help");

    CLI::App* tlsSub = app.add_subcommand("tls", "SNI Certificate");

    CLI::App* sniCertSub = tlsSub->add_subcommand("snicert", "Domain for SNI certificate");

    std::string sniName;
    std::string certChainFile;
    std::string certKeyFile;
    std::string certKeyPassword;
    std::string caFile;
    std::string caDir;
    bool useDefaultCaDir = false;
    std::string cipherList;
    ssl_option_t sslOptions = 0;

    CLI::Option* sniOpt = sniCertSub->add_option("--sni", sniName, "SNI Name");
    sniOpt->type_name("[FQDN with Wildcards]");
    sniOpt->required();

    CLI::Option* certChainFileOpt = sniCertSub->add_option("--cert-chain", certChainFile, "Certificate chain file");
    certChainFileOpt->type_name("[PEM file]");

    CLI::Option* certKeyFileOpt = sniCertSub->add_option("--cert-key", certKeyFile, "Certificate key file");
    certKeyFileOpt->type_name("[PEM file]");

    CLI::Option* certKeyPasswordOpt =
        sniCertSub->add_option("--cert-key-password", certKeyPassword, "Password for the certificate key file");
    certKeyPasswordOpt->type_name("[Password]");

    CLI::Option* caFileOpt = sniCertSub->add_option("--ca-cert-file", caFile, "CA-certificate file");
    caFileOpt->type_name("[PEM file]");

    CLI::Option* caDirOpt = sniCertSub->add_option("--ca-cert-dir", caDir, "CA-certificate directory");
    caDirOpt->type_name("[Path]");

    [[maybe_unused]] CLI::Option* useDefaultCaDirFlg =
        sniCertSub->add_flag("--use-default-ca-dir", useDefaultCaDir, "Use default CA-directory");

    CLI::Option* sslCipherListOpt = sniCertSub->add_option("--ssl-cipher-list", cipherList, "SSL ciphers");
    sslCipherListOpt->type_name("SSL cipher list");

    CLI::Option* sslOptionsOpt = sniCertSub->add_option("--ssl-options", sslOptions, "SSL options");
    sslOptionsOpt->type_name("SSL Options");
    sslOptionsOpt->trigger_on_parse();

    std::map<std::string, std::map<std::string, std::any>> sniCerts;

    //    std::map<std::string, std::map<std::string, std::any>> sniCerts = {
    //        {"snodec.home.vchrist.at", {{"CertChain", certChainFile}, {"CertChainKey", keyFile}, {"Password", password}}},
    //        {"www.vchrist.at", {{"CertChain", certChainFile}, {"CertChainKey", keyFile}, {"Password", password}}}};

    sniCertSub->parse_complete_callback([&sniCerts,
                                         &sniCertSub,
                                         &sniName,
                                         &certChainFile,
                                         &certKeyFile,
                                         &certKeyPassword,
                                         &caFile,
                                         &caDir,
                                         &useDefaultCaDir,
                                         &cipherList,
                                         &sslOptions](void) -> void {
        std::cout << "SNICERT: Complete callback: >> " << sniName << " : " << certChainFile << std::endl;
        sniCertSub->clear();

        if (!sniName.empty()) {
            if (!certChainFile.empty()) {
                sniCerts[sniName]["CertChain"] = certChainFile;
            }

            if (!certKeyFile.empty()) {
                sniCerts[sniName]["CertChainKey"] = certKeyFile;
            }
            if (!certKeyPassword.empty()) {
                sniCerts[sniName]["Password"] = certKeyPassword;
            }
            if (!caFile.empty()) {
                sniCerts[sniName]["CaFile"] = caFile;
            }
            if (!caDir.empty()) {
                sniCerts[sniName]["CaDir"] = caDir;
            }
            sniCerts[sniName]["UseDefaultCaDir"] = useDefaultCaDir;
            if (!cipherList.empty()) {
                sniCerts[sniName]["CipherList"] = cipherList;
            }
            sniCerts[sniName]["SslOptions"] = sslOptions;
        }
    });

    tlsSub->final_callback([&sniCerts](void) -> void {
        std::cout << "SNI Cert Sub: Parsing Finished" << std::endl;

        for (const auto& [sniName, sniMap] : sniCerts) {
            std::cout << "Domain: " << sniName << std::endl;
            for (auto& [name, value] : sniMap) {
                if (name == "CertChain") {
                    std::cout << "  CertChain: " << std::any_cast<const std::string&>(value) << std::endl;
                } else if (name == "CertChainKey") {
                    std::cout << "  CertChainKey: " << std::any_cast<const std::string&>(value) << std::endl;
                } else if (name == "Password") {
                    std::cout << "  Password: " << std::any_cast<const std::string&>(value) << std::endl;
                } else if (name == "CaFile") {
                    std::cout << "  CaFile: " << std::any_cast<const std::string&>(value) << std::endl;
                } else if (name == "CaDir") {
                    std::cout << "  CaDir: " << std::any_cast<const std::string&>(value) << std::endl;
                } else if (name == "UseDefaultCaDir") {
                    std::cout << "  UseDefaultCaDir: " << std::any_cast<bool>(value) << std::endl;
                } else if (name == "CipherList") {
                    std::cout << "  CipherList: " << std::any_cast<const std::string&>(value) << std::endl;
                } else if (name == "SslOptions") {
                    std::cout << "  SslOptions: " << std::any_cast<ssl_option_t>(value) << std::endl;
                }
            }
        }
    });

    std::map<std::string, std::map<std::string, std::string>> sniCerts1;

    app.add_option("--cert", sniCerts1, "SNICertificates");

    CLI11_PARSE(app, argc, argv)

    std::cout << app.config_to_str(true, true) << std::endl;

    for (const auto& [sniName1, sniMap] : sniCerts1) {
        std::cout << "Domain: " << sniName1 << std::endl;
        for (auto& [name, value] : sniMap) {
            std::cout << "  Key: " << name << ", Value: " << value << std::endl;
            /*
                        if (name == "CertChain") {
                            std::cout << "  CertChain: " << std::any_cast<const std::string&>(value) << std::endl;
                        } else if (name == "CertChainKey") {
                            std::cout << "  CertChainKey: " << std::any_cast<const std::string&>(value) << std::endl;
                        } else if (name == "Password") {
                            std::cout << "  Password: " << std::any_cast<const std::string&>(value) << std::endl;
                        } else if (name == "CaFile") {
                            std::cout << "  CaFile: " << std::any_cast<const std::string&>(value) << std::endl;
                        } else if (name == "CaDir") {
                            std::cout << "  CaDir: " << std::any_cast<const std::string&>(value) << std::endl;
                        } else if (name == "UseDefaultCaDir") {
                            std::cout << "  UseDefaultCaDir: " << std::any_cast<bool>(value) << std::endl;
                        } else if (name == "CipherList") {
                            std::cout << "  CipherList: " << std::any_cast<const std::string&>(value) << std::endl;
                        } else if (name == "SslOptions") {
                            std::cout << "  SslOptions: " << std::any_cast<ssl_option_t>(value) << std::endl;
                        }
            */
        }
    }

    return 0;
}
