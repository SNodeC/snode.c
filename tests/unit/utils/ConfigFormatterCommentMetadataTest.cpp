#include "utils/Formatter.h"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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

    void requireParseable(const std::string& config) {
        CLI::ConfigINI parser;
        std::istringstream input{config};
        (void) parser.from_config(input);
    }

} // namespace

int main() {
    CLI::App app{"Synthetic formatter app"};
    app.name("demo");
    app.config_formatter(std::make_shared<CLI::ConfigFormatter>());

    int port = 0;
    auto* portOpt = app.add_option("--port", port, "Port number")->required()->group("Options (persistent)");
    portOpt->run_callback_for_default(false);
    std::string host = "localhost";
    auto* hostOpt = app.add_option("--host", host, "Host name")->default_str("localhost")->group("Custom Group");
    bool verbose = false;
    app.add_flag("--verbose", verbose, "Verbose mode")->configurable(false);
    auto* tokenOpt = app.add_option("--token", host, "Token value")->group("Custom Group");
    tokenOpt->needs(hostOpt);
    hostOpt->excludes(portOpt);
    std::string callbackValue;
    app.add_option("--callback-required", callbackValue, "Callback required")
        ->required()
        ->run_callback_for_default()
        ->group("Custom Group");

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

    auto* instance = app.add_subcommand("", "Anonymous instance");
    instance->group("Instances");
    int instanceValue = 1;
    instance->add_option("--instance-value", instanceValue, "Instance value")->default_str("1");

    auto* section = app.add_subcommand("", "Anonymous section");
    section->group("Sections");
    int sectionValue = 2;
    section->add_option("--section-value", sectionValue, "Section value")->default_str("2");

    const std::string config = app.config_to_str(true, true);

    requireContains(config, "#@ snodec.meta begin document\n#@ {\n#@   \"schema\": \"snodec.config.comment-meta\",");
    requireContains(config, "#@ snodec.meta begin node");
    requireContains(config, "#@   \"kind\": \"application\",");
    requireContains(config, "#@   \"path\": [],");
    requireContains(config, "#@   \"path\": [\"outer\"],");
    requireContains(config, "#@   \"path\": [\"outer\", \"inner\"],");
    requireContains(config, "#@   \"kind\": \"category\",");
    requireContains(config, "#@   \"kind\": \"section\",");
    requireContains(config, "#@   \"kind\": \"instance\",");
    requireContains(config, "#@   \"path\": [\"<anonymous-0>\"],");
    requireContains(config, "#@   \"path\": [\"<anonymous-1>\"],");
    requireContains(config, "#@   \"path\": [\"<anonymous-2>\"],");
    requireContains(config, "#@ snodec.meta begin group");
    requireContains(config, "#@   \"name\": \"Options (persistent)\",");
    requireContains(config, "#@   \"kind\": \"persistent\",");
    requireContains(config, "#@   \"name\": \"Custom Group\",");
    requireContains(config, "#@   \"kind\": \"custom\",");
    requireContains(config, "#@ snodec.meta begin option");
    requireContains(config, "#@   \"id\": \"port\",");
    requireContains(config, "#@   \"key\": \"port\",");
    requireContains(config, "#@     \"isMissingRequired\": true,");
    requireContains(config, "#@     \"cppDefault\": \"<REQUIRED>\",");
    requireContains(config, "#@     \"cppDefaultLiteral\": \"\\\"<REQUIRED>\\\"\",");
    requireContains(config, "#port=\"<REQUIRED>\"");
    requireContains(config, "#callback-required=\"\"");
    requireNotContains(config, "#callback-required=\"<REQUIRED>\"");
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
    requireParseable(config);

    const std::string noComments = app.config_to_str(true, false);
    requireNotContains(noComments, "#@ snodec.meta");
    requireNotContains(noComments, "Port number");
    requireContains(noComments, "#port=\"<REQUIRED>\"");
    requireParseable(noComments);

    CLI::App configuredApp{"Configured default app"};
    configuredApp.name("configured");
    configuredApp.config_formatter(std::make_shared<CLI::ConfigFormatter>());
    std::string mode = "default";
    configuredApp.add_option("--mode", mode, "Mode value")->default_str("default");
    configuredApp.parse("--mode default");
    const std::string configuredConfig = configuredApp.config_to_str(true, true);
    requireContains(configuredConfig, "#@   \"key\": \"mode\",");
    requireContains(configuredConfig, "#@     \"configured\": \"default\",");
    requireContains(configuredConfig, "#@     \"configuredLiteral\": \"\\\"default\\\"\",");
    requireContains(configuredConfig, "#@     \"source\": \"command-line-or-config\",");
    requireContains(configuredConfig, "#@     \"isExplicitlyConfigured\": true,");

    CLI::App quotedApp{"Quoted value app"};
    quotedApp.name("quoted");
    quotedApp.config_formatter(std::make_shared<CLI::ConfigFormatter>());
    std::string text;
    const std::string configuredText = "a \"quoted\" value with \\ slash and spaces";
    quotedApp.add_option("--text", text, "Text value")->default_str("default \"quoted\" \\ value");
    quotedApp.parse(std::vector<std::string>{configuredText, "--text"});
    const std::string quotedConfig = quotedApp.config_to_str(true, true);
    requireContains(quotedConfig, "#@     \"configuredLiteral\": ");
    requireContains(quotedConfig, "\\\\ slash and spaces");

    CLI::App siblingApp{"Sibling anonymous app"};
    siblingApp.name("siblings");
    siblingApp.config_formatter(std::make_shared<CLI::ConfigFormatter>());
    auto* anonA = siblingApp.add_subcommand("", "First anonymous");
    anonA->group("Anon A");
    int a = 1;
    anonA->add_option("--value", a, "Shared value")->default_str("1");
    auto* anonB = siblingApp.add_subcommand("", "Second anonymous");
    anonB->group("Anon B");
    int b = 2;
    anonB->add_option("--value-b", b, "Shared value")->default_str("2");
    const std::string siblingConfig = siblingApp.config_to_str(true, true);
    requireContains(siblingConfig, "#@   \"id\": \"<anonymous-0>.value\",");
    requireContains(siblingConfig, "#@   \"id\": \"<anonymous-1>.value-b\",");
    requireContains(siblingConfig, "#@   \"key\": \"value\",");
    requireContains(siblingConfig, "#@   \"key\": \"value-b\",");

    CLI::App customFormatterApp{"Custom formatter app"};
    customFormatterApp.name("custom-format");
    auto customFormatter = std::make_shared<CLI::ConfigFormatter>();
    customFormatter->comment(';')->valueSeparator(':')->quoteCharacter('`', '!');
    customFormatterApp.config_formatter(customFormatter);
    std::string custom = "hello world";
    customFormatterApp.add_option("--custom", custom, "Custom value")->default_str("hello world");
    const std::string customConfig = customFormatterApp.config_to_str(true, false);
    requireContains(customConfig, ";custom:`hello world`");
    requireNotContains(customConfig, "#custom=");

    return 0;
}
