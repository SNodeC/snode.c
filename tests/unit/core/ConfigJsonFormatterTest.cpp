#include "utils/ConfigJsonFormatter.h"
#include "utils/Formatter.h"
#include "utils/JsonWriter.h"

#include "support/TestResult.h"

#include <sstream>
#include <string>

namespace {

    bool appearsBefore(const std::string& text, const std::string& left, const std::string& right) {
        const std::string::size_type leftPos = text.find(left);
        const std::string::size_type rightPos = text.find(right);

        return leftPos != std::string::npos && rightPos != std::string::npos && leftPos < rightPos;
    }

    void testJsonWriter(tests::support::TestResult& result) {
        std::ostringstream escaped;
        utils::JsonWriter writer(escaped);

        writer.beginArray();
        writer.string("plain");
        writer.string("quote\"");
        writer.string("back\\slash");
        writer.string("line\n tab\t cr\r");
        writer.string(std::string("ctrl") + static_cast<char>(0x01) + static_cast<char>(0x1f));
        writer.string("Grüße");
        writer.endArray();

        result.expectTrue(escaped.str() ==
                              std::string("[\"plain\",\"quote\\\"\",\"back\\\\slash\",\"line\\n tab\\t cr\\r\","
                                          "\"ctrl\\u0001\\u001f\",\"Grüße\"]"),
                          "JSON writer escapes strings deterministically");
    }

    void testExactSmallJson(tests::support::TestResult& result) {
        CLI::App app("Tiny desc", "tiny");
        app.configurable(false);

        std::string name = "default";
        CLI::Option* option = app.add_option("--name", name, "Name option")->type_name("string")->default_val("default")->configurable(true);
        option->group("Options");

        const std::string expected =
            "{\"format\":{\"name\":\"snodec.config\",\"version\":1,\"scope\":\"configurable-options-only\"},"
            "\"application\":{\"name\":\"tiny\",\"description\":\"Tiny desc\",\"version\":\"\"},"
            "\"tree\":{\"id\":\"app\",\"kind\":\"application\",\"kindSource\":\"root\",\"name\":\"tiny\","
            "\"displayName\":\"Application\",\"group\":\"SUBCOMMANDS\",\"description\":\"Tiny desc\",\"configurable\":false,"
            "\"configurableSource\":\"cli11\",\"required\":false,\"requiredSource\":\"cli11\",\"disabled\":false,"
            "\"disabledSource\":\"cli11\",\"path\":[],\"optionGroups\":[{\"name\":\"Options\",\"kind\":\"default\","
            "\"kindSource\":\"cli11-default-group\",\"options\":[{\"id\":\"name\",\"key\":\"name\",\"displayName\":\"name\","
            "\"description\":\"Name option\",\"configurable\":true,\"configurableSource\":\"cli11\",\"persistent\":true,"
            "\"persistentSource\":\"cli11-default-group\",\"required\":false,\"requiredSource\":\"cli11\",\"disabled\":false,"
            "\"commandLine\":{\"long\":\"--name\",\"short\":null,\"takesValue\":true,\"valueSeparator\":\" \",\"repeatable\":false},"
            "\"configFile\":{\"key\":\"name\",\"section\":null,\"writable\":true,\"writableSource\":\"cli11-configurable\"},"
            "\"type\":{\"kind\":\"string\",\"kindSource\":\"fallback\",\"name\":\"string\",\"cpp\":null,\"items\":\"single\"},"
            "\"constraints\":[],\"relations\":{\"needs\":[],\"excludes\":[]},"
            "\"value\":{\"apiDefault\":\"default\",\"configured\":null,\"effective\":\"default\",\"source\":\"api-default\","
            "\"isEffectiveDefault\":true,\"isExplicitlyConfigured\":false,\"isMissingRequired\":false,"
            "\"apiDefaultLiteral\":\"\\\"default\\\"\",\"configuredLiteral\":null,\"effectiveLiteral\":\"\\\"default\\\"\"}}]}],"
            "\"children\":[]}}\n";

        result.expectTrue(utils::ConfigJsonFormatter().toConfig(&app) == expected, "small JSON document matches exact expected output");
    }

    void testFormatter(tests::support::TestResult& result) {
        CLI::App app("Root description", "test-app");
        app.configurable(false);

        int port = 0;
        CLI::Option* portOpt = app.add_option("--port", port, "Port number")->type_name("port")->required()->configurable(true);
        portOpt->group("Options (persistent)");

        std::string mode = "default";
        CLI::Option* modeOpt = app.add_option("--mode", mode, "Mode option")->default_val("default")->configurable(true);
        modeOpt->group("Options (persistent)");

        std::string hidden = "hidden";
        app.add_option("--hidden", hidden, "Hidden option")->configurable(false);

        CLI::Option* needed = app.add_flag("--needed", "Needed flag")->configurable(true);
        needed->needs(modeOpt);
        needed->excludes(portOpt);

        CLI::App* category = app.add_subcommand("category", "Generic category");
        category->group("Categories")->configurable(false);

        CLI::App* nested = category->add_subcommand("nested", "Nested subcommand");
        nested->group("Custom")->configurable(false);

        bool flag = false;
        nested->add_flag("--flag", flag, "Nested flag")->configurable(true)->group("Options (persistent)");

        app.parse("--port 123 --mode configured");

        CLI::Option* requiredOpt = app.add_option("--required-value", hidden, "Required value")->required()->configurable(true);
        requiredOpt->group("Options (persistent)");

        const std::string json = utils::ConfigJsonFormatter().toConfig(&app);

        result.expectTrue(appearsBefore(json, "\"format\":", "\"application\":"), "JSON top-level order starts with format");
        result.expectTrue(appearsBefore(json, "\"application\":", "\"tree\":"), "JSON top-level order keeps tree after application");
        result.expectTrue(json.find("\"format\":{\"name\":\"snodec.config\",\"version\":1,\"scope\":\"configurable-options-only\"}") !=
                              std::string::npos,
                          "JSON includes canonical format and scope marker");
        result.expectTrue(json.find("\"application\":{\"name\":\"test-app\"") != std::string::npos, "JSON includes application name");
        result.expectTrue(json.find("\"tree\":{\"id\":\"app\",\"kind\":\"application\",\"kindSource\":\"root\"") !=
                              std::string::npos,
                          "JSON includes root tree node with source metadata");
        result.expectTrue(json.find("\"children\":[{\"id\":\"category\",\"kind\":\"category\",\"kindSource\":\"heuristic-cli11-group\"") !=
                              std::string::npos,
                          "JSON includes arbitrary child nodes with heuristic source metadata");
        result.expectTrue(json.find("\"id\":\"category.nested\"") != std::string::npos, "JSON includes nested child nodes");
        result.expectTrue(json.find("\"name\":\"Options (persistent)\",\"kind\":\"persistent\",\"kindSource\":\"heuristic-group-name\"") !=
                              std::string::npos,
                          "JSON preserves option groups with source metadata");
        result.expectTrue(json.find("\"key\":\"hidden\"") == std::string::npos, "JSON excludes non-configurable options");
        result.expectTrue(json.find("\"apiDefault\":\"<REQUIRED>\"") != std::string::npos, "JSON includes required placeholder");
        result.expectTrue(json.find("\"isMissingRequired\":true") != std::string::npos, "JSON marks missing required values");
        result.expectTrue(json.find("\"configured\":\"configured\"") != std::string::npos, "JSON includes semantic configured value");
        result.expectTrue(json.find("\"effective\":\"configured\"") != std::string::npos, "JSON includes semantic effective value");
        result.expectTrue(json.find("\"configuredLiteral\":\"\\\"configured\\\"\"") != std::string::npos,
                          "JSON includes explicit configured literal value");
        result.expectTrue(json.find("\"effectiveLiteral\":\"\\\"configured\\\"\"") != std::string::npos,
                          "JSON includes explicit effective literal value");
        result.expectTrue(json.find("\"isExplicitlyConfigured\":true") != std::string::npos, "JSON marks explicit configuration");
        result.expectTrue(json.find("\"kind\":\"integer\",\"kindSource\":\"heuristic-name\"") != std::string::npos,
                          "JSON marks heuristic type information");
        result.expectTrue(json.find("\"needs\":[\"mode\"]") != std::string::npos, "JSON includes needs relations");
        result.expectTrue(json.find("\"excludes\":[\"port\"]") != std::string::npos, "JSON includes excludes relations");

        const std::string ini = app.config_to_str(true, true);
        result.expectTrue(ini.find("port") != std::string::npos, "INI formatter still emits options");
    }

    void testValueFlags(tests::support::TestResult& result) {
        CLI::App defaultApp("Default", "default-app");
        std::string defaultValue = "same";
        defaultApp.add_option("--value", defaultValue)->default_val("same")->configurable(true);
        const std::string defaultJson = utils::ConfigJsonFormatter().toConfig(&defaultApp);
        result.expectTrue(defaultJson.find("\"isExplicitlyConfigured\":false") != std::string::npos,
                          "default-only value is not explicitly configured");
        result.expectTrue(defaultJson.find("\"isEffectiveDefault\":true") != std::string::npos,
                          "default-only value is effectively default");

        CLI::App differentApp("Different", "different-app");
        std::string differentValue = "same";
        differentApp.add_option("--value", differentValue)->default_val("same")->configurable(true);
        differentApp.parse("--value other");
        const std::string differentJson = utils::ConfigJsonFormatter().toConfig(&differentApp);
        result.expectTrue(differentJson.find("\"configured\":\"other\"") != std::string::npos,
                          "explicit different value is semantic without INI quotes");
        result.expectTrue(differentJson.find("\"isExplicitlyConfigured\":true") != std::string::npos,
                          "explicit different value is explicitly configured");
        result.expectTrue(differentJson.find("\"isEffectiveDefault\":false") != std::string::npos,
                          "explicit different value is not effectively default");

        CLI::App sameApp("Same", "same-app");
        std::string sameValue = "same";
        sameApp.add_option("--value", sameValue)->default_val("same")->configurable(true);
        sameApp.parse("--value same");
        const std::string sameJson = utils::ConfigJsonFormatter().toConfig(&sameApp);
        result.expectTrue(sameJson.find("\"isExplicitlyConfigured\":true") != std::string::npos,
                          "explicit default value is explicitly configured");
        result.expectTrue(sameJson.find("\"isEffectiveDefault\":true") != std::string::npos,
                          "explicit default value is still effectively default");
    }

    void testAnonymousNodeIds(tests::support::TestResult& result) {
        CLI::App app("Anonymous root", "anon-app");
        app.configurable(false);

        CLI::App* firstAnonymous = app.add_subcommand("", "First anonymous");
        firstAnonymous->group("Anonymous Group");
        int firstPort = 0;
        firstAnonymous->add_option("--first-port", firstPort, "First port")->configurable(true);

        CLI::App* secondAnonymous = app.add_subcommand("", "Second anonymous");
        secondAnonymous->group("Anonymous Group");
        int secondPort = 0;
        secondAnonymous->add_option("--second-port", secondPort, "Second port")->configurable(true);

        CLI::App* named = app.add_subcommand("named", "Named child");
        CLI::App* nestedAnonymous = named->add_subcommand("", "Nested anonymous");
        nestedAnonymous->group("Nested Anonymous Group");

        const std::string json = utils::ConfigJsonFormatter().toConfig(&app);

        result.expectTrue(json.find("\"id\":\"<anonymous-0>\"") != std::string::npos, "first anonymous child has generated ID");
        result.expectTrue(json.find("\"id\":\"<anonymous-1>\"") != std::string::npos, "second anonymous child has distinct generated ID");
        result.expectTrue(json.find("\"id\":\"named\"") != std::string::npos, "named sibling keeps named ID");
        result.expectTrue(json.find("\"id\":\"named.<anonymous-0>\"") != std::string::npos,
                          "nested anonymous child remains reachable");
        result.expectTrue(json.find("\"path\":[\"<anonymous-0>\"]") != std::string::npos,
                          "anonymous path contains generated segment");
        result.expectTrue(json.find("\"displayName\":\"Anonymous Group\"") != std::string::npos,
                          "anonymous display name uses group fallback");
        result.expectTrue(json.find("\"id\":\"<anonymous-0>.first-port\",\"key\":\"first-port\"") != std::string::npos,
                          "first anonymous option has unique tree ID and flattened config key");
        result.expectTrue(json.find("\"id\":\"<anonymous-1>.second-port\",\"key\":\"second-port\"") != std::string::npos,
                          "second anonymous option has distinct tree ID and flattened config key");
    }

    void testShowConfigOptionCompatibility(tests::support::TestResult& result) {
        std::string format = "ini";
        CLI::App app("Compatibility", "compat");
        CLI::Option* showConfig = app.add_flag("-s{ini},--show-config{ini}", format, "Show current configuration and exit")
                                     ->take_first()
                                     ->check(CLI::IsMember({"ini", "json"}));

        app.parse("-s");
        result.expectTrue(showConfig->count() == 1 && format == "ini", "-s defaults to INI");

        app.clear();
        format = "ini";
        app.parse("--show-config");
        result.expectTrue(showConfig->count() == 1 && format == "ini", "--show-config defaults to INI");

        app.clear();
        format = "ini";
        app.parse("--show-config=ini");
        result.expectTrue(showConfig->count() == 1 && format == "ini", "--show-config=ini selects INI");

        app.clear();
        format = "ini";
        app.parse("--show-config=json");
        result.expectTrue(showConfig->count() == 1 && format == "json", "--show-config=json selects JSON");

        app.clear();
        format = "ini";
        bool rejected = false;
        try {
            app.parse("--show-config=xml");
        } catch (const CLI::ParseError&) {
            rejected = true;
        }
        result.expectTrue(rejected, "--show-config rejects invalid formats");
    }

} // namespace

int main() {
    tests::support::TestResult result;

    testJsonWriter(result);
    testExactSmallJson(result);
    testFormatter(result);
    testValueFlags(result);
    testAnonymousNodeIds(result);
    testShowConfigOptionCompatibility(result);

    return result.processResult();
}
