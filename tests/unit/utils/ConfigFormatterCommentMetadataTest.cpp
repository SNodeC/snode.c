#include "utils/Formatter.h"
#include "utils/SubCommand.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

    class PromotionApp : public utils::SubCommand {
    public:
        PromotionApp(utils::SubCommand* parent = nullptr, const std::string& name = "promotion")
            : SubCommand(parent, std::make_shared<utils::AppWithPtr>("Promotion", name, this), "") {
        }
        using SubCommand::parse;
    };

    std::string extensiveConfig(const CLI::App& app) {
        const auto formatter = std::dynamic_pointer_cast<CLI::ConfigFormatter>(app.get_config_formatter());

        return formatter->to_config(&app, true, true, "", true);
    }

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

    const std::string config = extensiveConfig(app);

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
    const auto nodeBegin = config.find("#@ snodec.meta begin node\n");
    const auto nodeEnd = config.find("#@ snodec.meta end node\n", nodeBegin);
    const std::string nodeMetadata = config.substr(nodeBegin, nodeEnd - nodeBegin);
    for (const std::string& field : {"name", "configPrefix", "group", "description", "configurable", "required"}) {
        requireNotContains(nodeMetadata, "\"" + field + "\":");
    }
    requireContains(config, "#@ snodec.meta begin group");
    requireContains(config, "#@   \"name\": \"Options (persistent)\",");
    requireContains(config, "#@   \"kind\": \"persistent\",");
    requireContains(config, "#@   \"name\": \"Custom Group\",");
    requireContains(config, "#@   \"kind\": \"custom\",");
    const auto groupBegin = config.find("#@ snodec.meta begin group\n");
    const auto groupEnd = config.find("#@ snodec.meta end group\n", groupBegin);
    requireNotContains(config.substr(groupBegin, groupEnd - groupBegin), "\"kindSource\":");
    requireContains(config, "#@ snodec.meta begin option");
    requireContains(config, "#@   \"key\": \"port\",");
    requireContains(config, "#port=\"<REQUIRED>\"");
    requireContains(config, "#callback-required=\"\"");
    requireNotContains(config, "#callback-required=\"<REQUIRED>\"");
    for (const std::string& field : {"entity",
                                     "application",
                                     "scope",
                                     "syntax",
                                     "id",
                                     "localName",
                                     "configPrefix",
                                     "configurable",
                                     "effective",
                                     "persistent",
                                     "commandLine",
                                     "configFile",
                                     "constraints",
                                     "value"}) {
        requireNotContains(config, "\"" + field + "\":");
    }
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
                              "#@   \"version\": 1\n"
                              "#@ }\n"
                              "#@ snodec.meta end document\n";
    requireContains(config, exact);
    requireParseable(config);
    const std::string plain = app.config_to_str(true, true);
    requireNotContains(plain, "#@");
    requireContains(plain, "# Port number\n#port=\"<REQUIRED>\"");
    requireContains(plain, "#host=\"localhost\"");
    requireParseable(plain);
    std::istringstream metadataInput(config);
    std::string withoutMetadata;
    std::string line;
    while (std::getline(metadataInput, line)) {
        if (line.rfind("#@", 0) != 0) {
            withoutMetadata += line + '\n';
        }
    }
    if (withoutMetadata != plain) {
        std::abort();
    }

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
    const std::string configuredConfig = extensiveConfig(configuredApp);
    requireContains(configuredConfig, "#@   \"key\": \"mode\",");
    requireContains(configuredConfig, "#mode=\"default\"");
    requireNotContains(configuredConfig, "\nmode=");

    CLI::App quotedApp{"Quoted value app"};
    quotedApp.name("quoted");
    quotedApp.config_formatter(std::make_shared<CLI::ConfigFormatter>());
    std::string text;
    const std::string configuredText = "a \"quoted\" value with \\ slash and spaces";
    quotedApp.add_option("--text", text, "Text value")->default_str("default \"quoted\" \\ value");
    quotedApp.parse(std::vector<std::string>{configuredText, "--text"});
    const std::string quotedConfig = extensiveConfig(quotedApp);
    CLI::App restoredQuoted;
    restoredQuoted.config_formatter(std::make_shared<CLI::ConfigFormatter>());
    std::string restoredText;
    restoredQuoted.add_option("--text", restoredText);
    std::istringstream quotedInput(quotedConfig);
    restoredQuoted.parse_from_stream(quotedInput);
    if (restoredText != configuredText) {
        std::abort();
    }

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
    const std::string siblingConfig = extensiveConfig(siblingApp);
    requireContains(siblingConfig, "#@   \"nodePath\": [\"<anonymous-0>\"],");
    requireContains(siblingConfig, "#@   \"nodePath\": [\"<anonymous-1>\"],");
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

    CLI::App parentApp{"Parent app"};
    parentApp.config_formatter(std::make_shared<CLI::ConfigFormatter>());
    std::string inheritedValue;
    parentApp.add_option("--shared", inheritedValue, "Inherited option")->default_str("parent")->group("Shared");
    auto* child = parentApp.add_subcommand("child", "Child app")->fallthrough();
    std::string localValue;
    child->add_option("--shared", localValue, "Local hidden option")->configurable(false)->group("Shared");
    // CLI11 filters before resolving parent duplicates. Preserve the existing
    // heading, but do not serialize a parent option hidden by the local option.
    const std::string childConfig = child->config_to_str(true, true);
    requireContains(childConfig, "## Shared\n");
    requireNotContains(childConfig, "shared=");
    const std::string childExtensive = extensiveConfig(*child);
    requireContains(childExtensive, "#@ snodec.meta begin group");
    requireNotContains(childExtensive, "#@ snodec.meta begin option");
    requireNotContains(childExtensive, "shared=");

    auto* parsedChild = parentApp.add_subcommand("parsed", "Parsed child")->configurable();
    parsedChild->add_option("--value", localValue)->default_str("nested");
    parsedChild->parse("");
    const std::string parsedTreeConfig = extensiveConfig(parentApp);
    const std::string documentMarker = "#@ snodec.meta begin document\n";
    requireContains(parsedTreeConfig, documentMarker);
    if (parsedTreeConfig.find(documentMarker) != parsedTreeConfig.rfind(documentMarker)) {
        std::abort();
    }

    PromotionApp promotion;
    std::string bound;
    auto* boundOption =
        promotion.addOptionVariable("--bound", bound, "Bound value", "TEXT", std::string{"api"}, CLI::TypeValidator<std::string>());
    auto* untouched = promotion.addOption("--untouched", "Untouched", "INT", 9, CLI::TypeValidator<int>());
    auto* enabled = promotion.addFlag("--enabled{true}", "Enabled", "BOOL", "false", CLI::IsMember({"true", "false"}));
    std::string empty;
    promotion.addOptionVariable("--empty", empty, "Empty", "TEXT", std::string{}, CLI::TypeValidator<std::string>());
    std::vector<int> values;
    promotion.addOptionVariable("--values", values, "Values", "INT", std::vector<int>{2, 3}, CLI::Validator{});
    promotion.getAppWithPtr()->parse("--untouched 9");
    const std::string beforePromotion = promotion.configToStr();
    requireNotContains(beforePromotion, "\nbound=\"api\"\n");
    requireNotContains(beforePromotion, "\nuntouched=9\n");
    if (untouched->count() != 1 || boundOption->count() != 0) {
        std::abort();
    }
    promotion.option("--bound")->promoteDefault();
    promotion.option("--enabled")->promoteDefault();
    promotion.option("--empty")->promoteDefault();
    promotion.option("--values")->promoteDefault();
    const std::string promoted = promotion.configToStr();
    requireContains(promoted, "\n#bound=\"api\"\nbound=\"api\"\n");
    requireContains(promoted, "\nenabled=false\n");
    requireContains(promoted, "\nempty=\"\"\n");
    requireNotContains(promoted, "\nuntouched=9\n");
    if (bound != "api" || boundOption->count() != 1 || boundOption->get_default_str() != "api" || enabled->as<bool>()) {
        std::abort();
    }
    CLI::App restored;
    restored.config_formatter(std::make_shared<CLI::ConfigFormatter>());
    std::string restoredBound;
    std::string restoredEmpty = "different";
    bool restoredEnabled = true;
    std::vector<int> restoredValues;
    int restoredUntouched = 0;
    restored.add_option("--bound", restoredBound);
    restored.add_option("--empty", restoredEmpty);
    restored.add_flag("--enabled", restoredEnabled);
    restored.add_option("--values", restoredValues);
    restored.add_option("--untouched", restoredUntouched)->default_val(9);
    std::istringstream promotedInput(promoted);
    restored.parse_from_stream(promotedInput);
    if (restoredBound != "api" || !restoredEmpty.empty() || restoredEnabled || restoredValues != values || restoredUntouched != 9) {
        std::abort();
    }
    promotion.getAppWithPtr()->parse("--bound override");
    promotion.option("--bound")->promoteDefault();
    requireContains(promotion.configToStr(), "\nbound=\"override\"\n");
    if (bound != "override" || boundOption->get_default_str() != "api") {
        std::abort();
    }
    promotion.getAppWithPtr()->parse("--bound api");
    requireContains(promotion.configToStr(), "\nbound=\"api\"\n");
    promotion.getAppWithPtr()->parse("");
    if (boundOption->count() != 1 || bound != "api") {
        std::abort();
    }
    promotion.setDefaultValue(boundOption, std::string{"changed"});
    promotion.option("--bound")->promoteDefault();
    requireContains(promotion.configToStr(), "\nbound=\"changed\"\n");
    if (bound != "changed" || boundOption->get_default_str() != "changed") {
        std::abort();
    }
    promotion.getAppWithPtr()->parse("");
    requireContains(promotion.configToStr(), "\nbound=\"changed\"\n");

    // API setup before the first framework parse must not hide config/CLI input.
    PromotionApp startup;
    PromotionApp promotedSection{&startup, "section"};
    std::string startupValue;
    auto* startupOption = startup.addOptionVariable("--value", startupValue, "Value", "TEXT", std::string{"api"}, CLI::Validator{});
    auto* nestedOption = promotedSection.addOption("--nested", "Nested", "INT", 42, CLI::TypeValidator<int>());
    startup.option("--value")->promoteDefault();
    promotedSection.option("--nested")->promoteDefault();
    startup.getAppWithPtr()->set_config("--config");
    bool callbackSawResult = false;
    startup.finalCallback([&]() {
        callbackSawResult = startupOption->count() == 1 && nestedOption->count() == 1;
    });
    char program[] = "promotion";
    char* noArgs[]{program};
    // Use the framework's parse boundary for config-file precedence too.
    const auto filename = std::filesystem::temp_directory_path() / ("snodec-promotion-" + std::to_string(::getpid()) + ".ini");
    {
        std::ofstream file(filename);
        file << "value=from-file\n";
    }
    std::string filePath = filename.string();
    char configName[] = "--config";
    char* configArgs[]{program, configName, filePath.data()};
    startup.parse(3, configArgs);
    if (startupValue != "from-file") {
        std::abort();
    }
    startup.parse(1, noArgs);
    if (!callbackSawResult || startupValue != "api") {
        std::abort();
    }
    char optionName[] = "--value";
    char cliValue[] = "cli";
    char* cliArgs[]{program, optionName, cliValue};
    startup.parse(3, cliArgs);
    if (startupValue != "cli" || startupOption->get_default_str() != "api") {
        std::abort();
    }
    startup.parse(1, noArgs);
    if (startupValue != "api" || startupOption->count() != 1) {
        std::abort();
    }
    startup.parse(3, configArgs);
    if (startupValue != "from-file" || startupOption->count() != 1) {
        std::abort();
    }
    char* overrideArgs[]{program, configName, filePath.data(), optionName, cliValue};
    startup.parse(5, overrideArgs);
    if (startupValue != "cli") {
        std::abort();
    }
    std::filesystem::remove(filename);
    startup.setDefaultValue(startupOption, std::string{"updated"});
    startup.parse(1, noArgs);
    if (startupValue != "updated" || startupOption->get_default_str() != "updated") {
        std::abort();
    }
    requireContains(startup.configToStr(), "\nvalue=\"updated\"\n");

    PromotionApp outputModes;
    outputModes.addOption("--probe", "Probe", "INT", 7, CLI::TypeValidator<int>());
    for (const auto& [arguments, mode] : std::vector<std::pair<std::string, std::string>>{{"-s", "standard"},
                                                                                          {"--show-config", "standard"},
                                                                                          {"--show-config=standard", "standard"},
                                                                                          {"-S", "extensive"},
                                                                                          {"--show-config=extensive", "extensive"}}) {
        outputModes.getAppWithPtr()->parse(arguments);
        auto* option = outputModes.getOption("--show-config");
        if (option != outputModes.getOption("-s") || option != outputModes.getOption("-S") || option->count() != 1 ||
            option->as<std::string>() != mode) {
            std::abort();
        }
        requireNotContains(outputModes.configToStr(), "#@");
        requireContains(extensiveConfig(*outputModes.getAppWithPtr()), "#@ snodec.meta begin option");
    }
    bool invalidModeRejected = false;
    try {
        outputModes.getAppWithPtr()->parse("--show-config=invalid");
    } catch (const CLI::ParseError&) {
        invalidModeRejected = true;
    }
    if (!invalidModeRejected) {
        std::abort();
    }

    return 0;
}
