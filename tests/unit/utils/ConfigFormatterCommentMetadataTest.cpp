#include "utils/Formatter.h"

#include <iostream>
#include <sstream>
#include <string>

namespace {

    void requireContains(const std::string& text, const std::string& needle) {
        if (text.find(needle) == std::string::npos) {
            std::cerr << "Missing expected text:\n" << needle << "\n--- output ---\n" << text << std::endl;
            std::abort();
        }
    }

    void requireNotContains(const std::string& text, const std::string& needle) {
        if (text.find(needle) != std::string::npos) {
            std::cerr << "Unexpected text:\n" << needle << "\n--- output ---\n" << text << std::endl;
            std::abort();
        }
    }

    void requireBefore(const std::string& text, const std::string& first, const std::string& second) {
        const auto a = text.find(first);
        const auto b = text.find(second);
        if (a == std::string::npos || b == std::string::npos || a >= b) {
            std::cerr << "Ordering assertion failed:\n" << first << "\nbefore\n" << second << "\n--- output ---\n" << text << std::endl;
            std::abort();
        }
    }

} // namespace

int main() {
    CLI::App app{"Synthetic formatter app"};
    app.name("demo");
    app.config_formatter(std::make_shared<CLI::ConfigFormatter>());

    int port = 0;
    auto* portOpt = app.add_option("--port", port, "Port number")->required()->group("Options (persistent)");
    std::string host = "localhost";
    auto* hostOpt = app.add_option("--host", host, "Host name")->default_str("localhost")->group("Custom Group");
    bool verbose = false;
    app.add_flag("--verbose", verbose, "Verbose mode")->configurable(false);
    auto* tokenOpt = app.add_option("--token", host, "Token value")->group("Custom Group");
    tokenOpt->needs(hostOpt);
    hostOpt->excludes(portOpt);

    auto* anon = app.add_subcommand("", "Anonymous options");
    anon->group("Anonymous Group");
    int anonValue = 7;
    anon->add_option("--anon-value", anonValue, "Anonymous value")->default_str("7");

    auto* nested = app.add_subcommand("outer", "Outer node");
    nested->group("Custom Nodes");
    auto* inner = nested->add_subcommand("inner", "Inner node");
    inner->group("Sections");
    int depth = 3;
    inner->add_option("--depth", depth, "Nested depth")->default_str("3");

    const std::string config = app.config_to_str(true, true);

    requireContains(config, "#@ snodec.meta begin document\n#@ {\n#@   \"schema\": \"snodec.config.comment-meta\",");
    requireContains(config, "#@ snodec.meta begin node");
    requireContains(config, "#@   \"kind\": \"application\",");
    requireContains(config, "#@   \"path\": [\"demo\"],");
    requireContains(config, "#@   \"kind\": \"category\",");
    requireContains(config, "#@   \"kind\": \"section\",");
    requireContains(config, "#@   \"path\": [\"demo\", \"<anonymous-0>\"],");
    requireContains(config, "#@ snodec.meta begin group");
    requireContains(config, "#@   \"name\": \"Options (persistent)\",");
    requireContains(config, "#@   \"kind\": \"persistent\",");
    requireContains(config, "#@   \"name\": \"Custom Group\",");
    requireContains(config, "#@   \"kind\": \"custom\",");
    requireContains(config, "#@ snodec.meta begin option");
    requireContains(config, "#@   \"key\": \"port\",");
    requireContains(config, "#@     \"isMissingRequired\": true,");
    requireContains(config, "#@     \"cppDefault\": \"<REQUIRED>\",");
    requireContains(config, "#@     \"cppDefaultLiteral\": \"\\\"<REQUIRED>\\\"\",");
    requireContains(config, "#port=\"<REQUIRED>\"");
    requireContains(config, "#@     \"registrationDefault\": null,");
    requireContains(config, "#@     \"registrationDefaultSource\": \"not-tracked\",");
    requireNotContains(config, "apiDefault");
    requireContains(config, "#@       \"host\"");
    requireContains(config, "#@       \"port\"");
    requireContains(config, "#host=\"localhost\"");
    requireContains(config, "#outer.inner.depth=3");
    requireNotContains(config, "verbose=");
    requireBefore(config, "#@   \"key\": \"port\",", "# Port number\n#port=\"<REQUIRED>\"");
    requireBefore(config, "#@ snodec.meta begin group", "## Options (persistent)");

    const std::string exact = "#@ snodec.meta begin document\n"
                              "#@ {\n"
                              "#@   \"schema\": \"snodec.config.comment-meta\",\n"
                              "#@   \"version\": 1,\n"
                              "#@   \"entity\": \"document\",\n";
    requireContains(config, exact);

    CLI::ConfigINI parser;
    std::istringstream input{config};
    (void) parser.from_config(input);

    return 0;
}
