#include "log/Logger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#ifdef __has_warning
#if __has_warning("-Wweak-vtables")
#pragma GCC diagnostic ignored "-Wweak-vtables"
#endif
#if __has_warning("-Wcovered-switch-default")
#pragma GCC diagnostic ignored "-Wcovered-switch-default"
#endif
#endif
#endif
#include "utils/CLI11.hpp"
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

#endif // DOXYGEN_SHOULD_SKIP_THIS

#include <string>

int main(int argc, char* argv[]) {
    CLI::App app("ConfigTest");

    CLI::Option* allHelpOpt = app.set_help_all_flag("--help-all", "Expand all help");
    allHelpOpt->configurable(false);

    CLI::App* subApp = app.add_subcommand("test", "Test SubCommand");

    std::string filename;
    CLI::Option* filenameOpt = subApp->add_option("-f", filename, "A Filename");
    //    filenameOpt->default_val("Filenameeeeee");

    VLOG(1) << "Filename: " << filename;

    //    app.needs(subApp);
    //    subApp->needs(filenameOpt);
    subApp->required();
    filenameOpt->required();

    CLI11_PARSE(app, argc, argv)

    return 0;
}
