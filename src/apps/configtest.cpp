#include "log/Logger.h"
#include "utils/CLI11.hpp"

#include <string> // for string, allocator

int main(int argc, char* argv[]) {
    CLI::App app("ConfigTest");

    CLI::Option* allHelpOpt = app.set_help_all_flag("--help-all", "Expand all help");
    allHelpOpt->configurable(false);

    CLI::App* subApp = app.add_subcommand("test", "Test SubCommand");

    std::string filename;
    CLI::Option* filenameOpt = subApp->add_option("-f", filename, "A Filename");
    //    filenameOpt->default_val("Filenameeeeee");

    VLOG(0) << "Filename: " << filename;

    //    app.needs(subApp);
    //    subApp->needs(filenameOpt);
    subApp->required();
    filenameOpt->required();

    CLI11_PARSE(app, argc, argv)

    return 0;
}
