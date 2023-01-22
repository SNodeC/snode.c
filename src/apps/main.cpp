#include "utils/CLI11.hpp"

#include <cstdint>
#include <iostream>
#include <memory>
#include <openssl/opensslv.h>
#include <string>

// IWYU pragma: no_include <bits/utility.h>

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
using ssl_option_t = uint64_t;
#else
using ssl_option_t = uint32_t;
#endif

int main(int argc, char** argv) {
    CLI::App app("data output specification");
    app.set_help_all_flag("--help-all", "Expand all help");
    app.get_config_formatter_base()->defaultAlsoPrefix('#');

    CLI::App* tlsSub = app.add_subcommand("tls", "SNI Certificate");

    CLI::App* sniCertSub = tlsSub->add_subcommand("snicert", "Domain for SNI certificate");
    sniCertSub->required();

    std::string sniName;

    CLI::Option* sniOpt = sniCertSub->add_option("--sni", sniName, "SNI Name");
    sniOpt->type_name("[FQDN with Wildcards]");
    sniOpt->default_val("Test SNIName");

    sniOpt->required();

    sniCertSub->disabled();

    sniOpt->default_val("Hihihi");
    //    sniOpt->add_result("hihihi");

    CLI11_PARSE(app, argc, argv)

    std::cout << app.config_to_str(true, true) << std::endl;

    return 0;
}
