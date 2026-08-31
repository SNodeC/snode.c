from pathlib import Path


def replace_once(path, old, new):
    p = Path(path)
    text = p.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected one match, found {count}: {old[:80]!r}")
    p.write_text(text.replace(old, new, 1))


replace_once(
    "src/utils/SubCommand.h",
    "    struct ConfigOptionState {\n        ConfigRequirementState required;\n    };",
    "    struct ConfigOptionState {\n        ConfigRequirementState required;\n        bool sensitive = false;\n    };",
)
replace_once(
    "src/utils/SubCommand.h",
    "        void setCanonicalRequired(CLI::Option* option, bool required);\n        void setSuppression(ConfigSuppressionReason reason, bool suppressed);",
    "        void setCanonicalRequired(CLI::Option* option, bool required);\n        void setSensitive(CLI::Option* option, bool sensitive);\n        void setSuppression(ConfigSuppressionReason reason, bool suppressed);",
)
replace_once(
    "src/utils/SubCommand.h",
    "        bool canonicalRequired(const CLI::Option* option) const;\n        bool effectiveRequired(const CLI::Option* option) const;",
    "        bool canonicalRequired(const CLI::Option* option) const;\n        bool effectiveRequired(const CLI::Option* option) const;\n        bool sensitive(const CLI::Option* option) const;",
)
replace_once(
    "src/utils/SubCommand.h",
    "        CLI::Option* setConfigurable(CLI::Option* option, bool configurable) const;\n",
    "        CLI::Option* setConfigurable(CLI::Option* option, bool configurable) const;\n        CLI::Option* setSensitive(CLI::Option* option, bool sensitive = true) const;\n",
)

replace_once(
    "src/utils/SubCommand.cpp",
    "#include <cstddef>\n#include <cstdint>\n#include <memory>\n#include <set>\n#include <vector>",
    "#include <algorithm>\n#include <cstddef>\n#include <cstdint>\n#include <memory>\n#include <set>\n#include <vector>",
)
replace_once(
    "src/utils/SubCommand.cpp",
    "namespace utils {\n\n    SubCommand::SubCommand",
    r'''namespace utils {

    namespace {
        std::string jsonEscapeSensitiveValue(const std::string& value) {
            std::string escaped;
            escaped.reserve(value.size());
            for (const char ch : value) {
                switch (ch) {
                    case '\\':
                        escaped += "\\\\";
                        break;
                    case '"':
                        escaped += "\\\"";
                        break;
                    case '\n':
                        escaped += "\\n";
                        break;
                    case '\r':
                        escaped += "\\r";
                        break;
                    case '\t':
                        escaped += "\\t";
                        break;
                    default:
                        escaped.push_back(ch);
                }
            }
            return escaped;
        }

        void addSensitiveValueVariants(std::set<std::string>& values, const std::string& value) {
            if (value.empty() || value == "<REQUIRED>" || value == "<REDACTED>") {
                return;
            }
            values.insert(value);
            values.insert(jsonEscapeSensitiveValue(value));
            values.insert(CLI::detail::convert_arg_for_ini(value, '"', '\'', true));
        }

        std::set<std::string> collectSensitiveOptionValues(CLI::App* app) {
            std::set<std::string> values;

            if (const auto* appWithPtr = dynamic_cast<const AppWithPtr*>(app); appWithPtr != nullptr) {
                for (const CLI::Option* option : app->get_options()) {
                    if (!appWithPtr->sensitive(option)) {
                        continue;
                    }

                    addSensitiveValueVariants(values, option->get_default_str());
                    for (const std::string& value : option->results()) {
                        addSensitiveValueVariants(values, value);
                    }
                    try {
                        for (const std::string& value : option->reduced_results()) {
                            addSensitiveValueVariants(values, value);
                        }
                    } catch (const CLI::ParseError&) {
                    }
                }
            }

            for (CLI::App* subCommand : app->get_subcommands({})) {
                const auto childValues = collectSensitiveOptionValues(subCommand);
                values.insert(childValues.begin(), childValues.end());
            }

            return values;
        }

        void replaceAll(std::string& text, const std::string& needle, const std::string& replacement) {
            if (needle.empty()) {
                return;
            }
            std::size_t pos = 0;
            while ((pos = text.find(needle, pos)) != std::string::npos) {
                text.replace(pos, needle.size(), replacement);
                pos += replacement.size();
            }
        }

        std::string redactSensitiveConfig(std::string config, const std::set<std::string>& sensitiveValues) {
            std::vector<std::string> ordered(sensitiveValues.begin(), sensitiveValues.end());
            std::sort(ordered.begin(), ordered.end(), [](const std::string& left, const std::string& right) {
                return left.size() > right.size();
            });
            for (const std::string& value : ordered) {
                replaceAll(config, value, "<REDACTED>");
            }
            return config;
        }
    } // namespace

    SubCommand::SubCommand''',
)
replace_once(
    "src/utils/SubCommand.cpp",
    "    std::string SubCommand::configToStr() const {\n        return subCommandApp->config_to_str(true, true);\n    }",
    "    std::string SubCommand::configToStr() const {\n        return redactSensitiveConfig(subCommandApp->config_to_str(true, true), collectSensitiveOptionValues(subCommandApp));\n    }",
)
replace_once(
    "src/utils/SubCommand.cpp",
    '''    CLI::Option* SubCommand::setConfigurable(CLI::Option* option, bool configurable) const {\n        return option //\n            ->configurable(configurable)\n            ->group(subCommandApp->get_formatter()->get_label(configurable ? "Persistent Options" : "Nonpersistent Options"));\n    }''',
    '''    CLI::Option* SubCommand::setConfigurable(CLI::Option* option, bool configurable) const {\n        return option //\n            ->configurable(configurable)\n            ->group(subCommandApp->get_formatter()->get_label(configurable ? "Persistent Options" : "Nonpersistent Options"));\n    }\n\n    CLI::Option* SubCommand::setSensitive(CLI::Option* option, bool sensitive) const {\n        subCommandApp->setSensitive(option, sensitive);\n        return option;\n    }''',
)
replace_once(
    "src/utils/SubCommand.cpp",
    "    void AppWithPtr::setCanonicalRequired(CLI::Option* option, bool required) {\n        optionState(option).required.canonicalRequired = required;\n        applyEffectiveState();\n    }",
    "    void AppWithPtr::setCanonicalRequired(CLI::Option* option, bool required) {\n        optionState(option).required.canonicalRequired = required;\n        applyEffectiveState();\n    }\n\n    void AppWithPtr::setSensitive(CLI::Option* option, bool sensitive) {\n        optionState(option).sensitive = sensitive;\n    }",
)
replace_once(
    "src/utils/SubCommand.cpp",
    "    bool AppWithPtr::effectiveRequired(const CLI::Option* option) const {\n        const ConfigOptionState* state = findOptionState(option);\n\n        return state != nullptr && state->required.effectiveRequired;\n    }",
    "    bool AppWithPtr::effectiveRequired(const CLI::Option* option) const {\n        const ConfigOptionState* state = findOptionState(option);\n\n        return state != nullptr && state->required.effectiveRequired;\n    }\n\n    bool AppWithPtr::sensitive(const CLI::Option* option) const {\n        const ConfigOptionState* state = findOptionState(option);\n\n        return state != nullptr && state->sensitive;\n    }",
)

replace_once(
    "src/utils/Config.cpp",
    "            for (const CLI::Option* option : app->get_options()) {\n                if (option->get_configurable()) {\n                    std::string value;\n\n                    switch (mode) {",
    "            for (const CLI::Option* option : app->get_options()) {\n                if (option->get_configurable()) {\n                    std::string value;\n                    const auto* appWithPtr = dynamic_cast<const AppWithPtr*>(app);\n                    const bool sensitive = appWithPtr != nullptr && appWithPtr->sensitive(option);\n\n                    switch (mode) {",
)
replace_once(
    "src/utils/Config.cpp",
    "                    if (!value.empty()) {\n                        if (value.starts_with(std::string{\"[\"}) && value.ends_with(\"]\")) {",
    "                    if (sensitive && !value.empty() && value != \"<REQUIRED>\" && value != \"\\\"\\\"\") {\n                        value = \"<REDACTED>\";\n                    }\n\n                    if (!value.empty()) {\n                        if (value.starts_with(std::string{\"[\"}) && value.ends_with(\"]\")) {",
)
replace_once(
    "src/utils/Config.cpp",
    "                        if (value != \"<REQUIRED>\" && value != \"\\\"\\\"\" && !value.starts_with(\"<[\")) {\n                            value = bash_backslash_escape_no_whitespace(value);\n                        }",
    "                        if (value != \"<REQUIRED>\" && value != \"<REDACTED>\" && value != \"\\\"\\\"\" && !value.starts_with(\"<[\")) {\n                            value = bash_backslash_escape_no_whitespace(value);\n                        }",
)

test = r'''#include "utils/SubCommand.h"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>

namespace {

    void require(bool condition, const std::string& message) {
        if (!condition) {
            std::cerr << message << std::endl;
            std::abort();
        }
    }

    class TestSubCommand : public utils::SubCommand {
    public:
        TestSubCommand(utils::SubCommand* parent, const std::string& name)
            : utils::SubCommand(parent, std::make_shared<utils::AppWithPtr>("test command", name, this), "Tests") {
        }
    };

} // namespace

int main() {
    TestSubCommand root{nullptr, "root"};
    const std::string secret = "s3cr3t-\\\"value";

    auto* secretOpt = root.addOption("--secret", "Sensitive value", "string", CLI::TypeValidator<std::string>());
    root.setConfigurable(secretOpt, true);
    root.setSensitive(secretOpt);
    root.setDefaultValue(secretOpt, secret);

    require(secretOpt->as<std::string>() == secret, "sensitive option no longer returns its real configured value");

    const std::string serialized = root.configToStr();
    require(serialized.find("s3cr3t") == std::string::npos, "config serialization exposed sensitive value");
    require(serialized.find("<REDACTED>") != std::string::npos, "config serialization did not mark sensitive value as redacted");

    root.setSensitive(secretOpt, false);
    const std::string unredacted = root.configToStr();
    require(unredacted.find("s3cr3t") != std::string::npos, "clearing sensitivity did not restore normal serialization");

    return EXIT_SUCCESS;
}
'''
Path("tests/unit/utils/SensitiveConfigOptionTest.cpp").write_text(test)

cmake_path = Path("tests/unit/utils/CMakeLists.txt")
cmake = cmake_path.read_text()
if "SensitiveConfigOptionTest" in cmake:
    raise SystemExit("SensitiveConfigOptionTest already registered unexpectedly")
cmake_path.write_text(
    cmake.rstrip()
    + '\n\nsnodec_add_test(SensitiveConfigOptionTest SensitiveConfigOptionTest.cpp)\n'
    + 'target_link_libraries(SensitiveConfigOptionTest PRIVATE snodec-test-support snodec::utils)\n'
    + 'set_tests_properties(SensitiveConfigOptionTest PROPERTIES LABELS "unit;utils;config;security" TIMEOUT 5)\n'
)
