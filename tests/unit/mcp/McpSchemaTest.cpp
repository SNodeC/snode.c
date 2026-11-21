// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "core/SNodeC.h"
#include "mcp/server/Server.h"
#include "src/apps/mcp/FindTool.h"
#include "src/apps/mcp/ReadTool.h"
#include "src/apps/mcp/SkeletonPreviewTool.h"
#include "tests/support/TestResult.h"
#include "utils/Config.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <sstream>
using mcp::server::Json;
int main(int argc, char* argv[]) {
    tests::support::TestResult test;
    const std::filesystem::path root(MCP_SOURCE_ROOT);
    const std::regex rawSchema("R\"json\\(([\\s\\S]*?)\\)json\"");
    std::size_t checked = 0;
    for (const auto* directory : {"src/apps/mcp", "tests/unit/mcp", "tests/component/mcp", "tests/installed-consumer", "docs/mcp"}) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root / directory)) {
            const auto extension = entry.path().extension();
            if (entry.is_regular_file() && (extension == ".cpp" || extension == ".h" || extension == ".md")) {
                std::ifstream stream(entry.path());
                const std::string source{std::istreambuf_iterator<char>(stream), {}};
                for (std::sregex_iterator it(source.begin(), source.end(), rawSchema), end; it != end; ++it) {
                    std::string text = std::regex_replace((*it)[1].str(), std::regex("^[ \t\r\n]*\n|\n[ \t\r\n]*$"), "");
                    std::size_t indent = std::string::npos;
                    std::istringstream lines(text);
                    for (std::string line; std::getline(lines, line);) {
                        indent = std::min(indent, line.find_first_not_of(' '));
                    }
                    text = std::regex_replace(text, std::regex("^ {" + std::to_string(indent) + "}", std::regex_constants::multiline), "");
                    test.expectTrue(Json::parse(text).dump(4) == text, entry.path().string() + " schema matches dump(4)");
                    ++checked;
                }
            }
        }
    }
    test.expectTrue(checked >= 20, "schema formatting check exercised production, fixtures and documentation");
    auto* config = utils::Config::configRoot.newSubCommand<apps::mcp::DevelopmentConfig>();
    config->setDefaultValue(config->getOption("--header-root"), (root / "src").string());
    core::SNodeC::init(argc, argv);
    const apps::mcp::FindTool find(*config);
    const apps::mcp::ReadTool read(*config);
    const apps::mcp::SkeletonPreviewTool preview;
    const Json previous = Json::parse(R"json(
{
    "api.find": {
        "additionalProperties": false,
        "properties": {
            "limit": {
                "default": 20,
                "maximum": 100,
                "minimum": 1,
                "type": "integer"
            },
            "query": {
                "maxLength": 128,
                "minLength": 1,
                "type": "string"
            }
        },
        "required": [
            "query"
        ],
        "type": "object"
    },
    "api.read": {
        "additionalProperties": false,
        "properties": {
            "bytes": {
                "maximum": 65536,
                "minimum": 1,
                "type": "integer"
            },
            "offset": {
                "maximum": 67108864,
                "minimum": 0,
                "type": "integer"
            },
            "path": {
                "maxLength": 512,
                "minLength": 1,
                "type": "string"
            }
        },
        "required": [
            "path"
        ],
        "type": "object"
    },
    "skeleton.preview": {
        "additionalProperties": false,
        "properties": {
            "kind": {
                "default": "application",
                "enum": [
                    "application",
                    "module"
                ]
            }
        },
        "type": "object"
    }
}
)json");
    const mcp::server::Tool* tools[] = {&find, &read, &preview};
    for (const auto* tool : tools) {
        const auto& definition = tool->getDefinition();
        test.expectTrue(definition.inputSchema == previous.at(definition.name),
                        definition.name + " schema equals pre-conversion wire value");
    }
    for (const std::string field : {"inputSchema", "outputSchema"}) {
        bool named = false;
        try {
            apps::mcp::parseSchema("broken", "{", field.c_str());
        } catch (const mcp::server::RegistrationError& error) {
            named = std::string(error.what()).find("tool broken: " + field + ":") != std::string::npos;
        }
        test.expectTrue(named, "schema parse error names the registration and " + field);
    }
    core::SNodeC::free();
    return test.processResult();
}
