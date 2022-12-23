#include "utils/CLI11.hpp"

#include <stdexcept>
#include <string>

int main(int argc, char** argv) {
    CLI::App app("Some nice discription");

    int x = 7;
    app.add_option("-x", x, "An integer value");

    std::string name;
    app.add_option("-n,--name", name, "Some required string");

    bool flag;
    app.add_flag("-f,--flag", flag, "A flag option");

    int y = 3;
    CLI::App* sub1 = app.add_subcommand("sub1", "Subcommand 1");
    sub1->add_option("--sub1_opt", y, "An integer value for sub1");

    int z = 4;
    CLI::App* sub2 = app.add_subcommand("sub2", "Subcommand 2");
    sub2->add_option("--sub2_opt", z, "An integer value for sub2");

    CLI11_PARSE(app, argc, argv);

    return 0;
}
