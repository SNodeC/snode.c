/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "core/SNodeC.h"
#include "mcp/Types.h"
#include "mcp/server/Server.h"
#include "src/apps/mcp/FindTool.h"
#include "src/apps/mcp/ReadTool.h"
#include "src/apps/mcp/SkeletonPreviewTool.h"
#include "src/apps/mcp/Tools.h"
#include "tests/support/TestResult.h"
#include "utils/Config.h"

#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <map>
#include <string>

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <regex>
#include <sstream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

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
                {
                    std::sregex_iterator it(source.begin(), source.end(), rawSchema);
                    std::sregex_iterator end;
                    for (; it != end; ++it) {
                        std::string text = std::regex_replace((*it)[1].str(), std::regex("^[ \t\r\n]*\n|\n[ \t\r\n]*$"), "");
                        std::size_t indent = std::string::npos;
                        std::istringstream lines(text);
                        for (std::string line; std::getline(lines, line);) {
                            indent = std::min(indent, line.find_first_not_of(' '));
                        }

                        text =
                            std::regex_replace(text, std::regex("^ {" + std::to_string(indent) + "}", std::regex_constants::multiline), "");
                        test.expectTrue(nlohmann::ordered_json::parse(text).dump(4) == text,
                                        entry.path().string() + " schema matches dump(4)");
                        ++checked;
                    }
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
    const Json previous = nlohmann::ordered_json::parse(R"json(
{
    "api.find": {
        "type": "object",
        "properties": {
            "query": {
                "type": "string",
                "maxLength": 128,
                "minLength": 1
            },
            "limit": {
                "type": "integer",
                "default": 20,
                "maximum": 100,
                "minimum": 1
            }
        },
        "required": [
            "query"
        ],
        "additionalProperties": false
    },
    "api.read": {
        "type": "object",
        "properties": {
            "path": {
                "type": "string",
                "maxLength": 512,
                "minLength": 1
            },
            "offset": {
                "type": "integer",
                "maximum": 67108864,
                "minimum": 0
            },
            "bytes": {
                "type": "integer",
                "maximum": 65536,
                "minimum": 1
            }
        },
        "required": [
            "path"
        ],
        "additionalProperties": false
    },
    "skeleton.preview": {
        "type": "object",
        "properties": {
            "kind": {
                "default": "application",
                "enum": [
                    "application",
                    "module"
                ]
            }
        },
        "additionalProperties": false
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
