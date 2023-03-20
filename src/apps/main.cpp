#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wweak-vtables"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#endif
#include "utils/CLI11.hpp" // IWYU pragma: export
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#include "utils/Formatter.h"

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
    std::shared_ptr<CLI::ConfigFormatter> configFormatter = std::make_shared<CLI::ConfigFormatter>();
    app.config_formatter(configFormatter);

    CLI::App* tlsSub = app.add_subcommand("tls", "SNI Certificate");
    tlsSub->required();

    //    sniOpt->required();

    // tlsSub->disabled();
    CLI::App* sniCertSub = tlsSub->add_subcommand("snicert", "Domain for SNI certificate");
    sniCertSub->required();

    std::string sniName;

    CLI::Option* sniOpt = sniCertSub->add_option("--sni", sniName, "SNI Name");
    sniOpt->type_name("[FQDN with Wildcards]");
    sniOpt->default_val("Test SNIName");

    //    sniOpt->required();

    // tlsSub->disabled();

    sniOpt->default_val("Hihihi");
    //    sniOpt->add_result("hihihi");

    //    CLI11_PARSE(app, argc, argv)

    app.parse(argc, argv);

    std::cout << app.config_to_str(true, true) << std::endl;

    return 0;
}
