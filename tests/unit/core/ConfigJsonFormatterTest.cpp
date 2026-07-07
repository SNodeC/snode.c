#include "utils/Formatter.h"

#include "support/TestResult.h"

#include <sstream>
#include <string>

int main() {
    tests::support::TestResult result;

    std::ostringstream escaped;
    CLI::JsonWriter writer(escaped);
    writer.beginArray();
    writer.string("plain");
    writer.string("quote\"");
    writer.string("back\\slash");
    writer.string("line\n tab\t cr\r");
    writer.string(std::string("ctrl") + static_cast<char>(0x01));
    writer.string("Grüße");
    writer.endArray();
    result.expectTrue(escaped.str() == std::string("[\"plain\",\"quote\\\"\",\"back\\\\slash\",\"line\\n tab\\t cr\\r\",\"ctrl\\u0001\",\"Grüße\"]"),
                      "JSON writer escapes strings deterministically");

    CLI::App app("Root description", "test-app");
    app.configurable(false);
    int port = 0;
    auto* portOpt = app.add_option("--port", port, "Port number")->type_name("port")->required()->configurable(true);
    portOpt->group("Options (persistent)");
    std::string mode = "default";
    app.add_option("--mode", mode, "Mode option")->default_val("default")->configurable(true)->group("Options (persistent)");
    auto* category = app.add_subcommand("category", "Generic category");
    category->group("Categories")->configurable(false);
    auto* nested = category->add_subcommand("nested", "Nested subcommand");
    nested->group("Custom")->configurable(false);
    bool flag = false;
    nested->add_flag("--flag", flag, "Nested flag")->configurable(true)->group("Options (persistent)");

    const std::string json = CLI::JsonConfigFormatter().to_json_config(&app);
    result.expectTrue(json.find("\"format\":{\"name\":\"snodec.config\",\"version\":1}") != std::string::npos,
                      "JSON includes canonical format marker");
    result.expectTrue(json.find("\"application\":{\"name\":\"test-app\"") != std::string::npos,
                      "JSON includes application name");
    result.expectTrue(json.find("\"tree\":") != std::string::npos, "JSON includes tree");
    result.expectTrue(json.find("\"children\":[{\"id\":\"category\"") != std::string::npos,
                      "JSON includes arbitrary child nodes");
    result.expectTrue(json.find("\"id\":\"category.nested\"") != std::string::npos,
                      "JSON includes nested child nodes");
    result.expectTrue(json.find("\"optionGroups\":[{\"name\":\"Options (persistent)\"") != std::string::npos,
                      "JSON includes option groups");
    result.expectTrue(json.find("\"apiDefault\":\"<REQUIRED>\"") != std::string::npos,
                      "JSON includes required placeholder");
    result.expectTrue(json.find("\"isMissingRequired\":true") != std::string::npos,
                      "JSON marks missing required values");
    result.expectTrue(json.find("\"effective\":\"default\"") != std::string::npos,
                      "JSON includes effective default value");

    const std::string ini = app.config_to_str(true, true);
    result.expectTrue(ini.find("port") != std::string::npos, "INI formatter still emits options");

    return result.processResult();
}
