/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
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

#include "ConfigParser.h"

#include <algorithm>
#include <iterator>
#include <regex>
#include <sstream>
#include <utility>

namespace snodec::control {

    namespace {

        constexpr char requiredPlaceholder[] = "<REQUIRED>";

        std::string trim(const std::string& value) {
            const std::size_t first = value.find_first_not_of(" \t\r\n");

            if (first == std::string::npos) {
                return "";
            }

            const std::size_t last = value.find_last_not_of(" \t\r\n");

            return value.substr(first, last - first + 1);
        }

        // Strips a single layer of surrounding double quotes and undoes the minimal backslash escaping
        // used by CLI11's INI writer (\" and \\). Unquoted tokens (numbers, bare words such as true or
        // false, or the empty string) are returned unchanged.
        std::string unquote(const std::string& raw) {
            if (raw.size() < 2 || raw.front() != '"' || raw.back() != '"') {
                return raw;
            }

            std::string result;
            result.reserve(raw.size() - 2);

            for (std::size_t i = 1; i + 1 < raw.size(); ++i) {
                if (raw[i] == '\\' && i + 2 < raw.size() && (raw[i + 1] == '"' || raw[i + 1] == '\\')) {
                    result.push_back(raw[i + 1]);
                    ++i;
                } else {
                    result.push_back(raw[i]);
                }
            }

            return result;
        }

        // Applies a parsed "key=value" line, either the commented default/template line or the active,
        // uncommented one, to the option identified by `name`, creating the option (and its section) on
        // first use.
        void applyKeyValue(
            ConfigModel& model, const std::string& name, const std::string& rawValue, bool isActiveLine, std::string& pendingDescription) {
            const auto [sectionName, key] = splitSectionAndKey(name);
            ConfigSection& section = model.sectionFor(sectionName);

            auto it = std::find_if(section.options.begin(), section.options.end(), [&key](const ConfigOption& option) {
                return option.key == key;
            });

            if (it == section.options.end()) {
                ConfigOption newOption;
                newOption.section = sectionName;
                newOption.key = key;
                section.options.push_back(std::move(newOption));
                it = std::prev(section.options.end());
            }

            if (it->description.empty() && !pendingDescription.empty()) {
                it->description = pendingDescription;
            }
            pendingDescription.clear();

            const std::string value = unquote(rawValue);

            if (isActiveLine) {
                it->activeValue = value;
                it->hasActiveValue = true;
                it->fromActiveLine = true;
            } else if (value == requiredPlaceholder) {
                it->required = true;
                it->defaultValue = std::nullopt;
                it->fromCommentedDefaultLine = true;
            } else {
                it->defaultValue = value;
                it->fromCommentedDefaultLine = true;
            }
        }

    } // namespace

    ParseResult parseShowConfigOutput(const std::string& text) {
        ParseResult result;

        static const std::regex keyValuePattern(R"(^([A-Za-z0-9_.\-]+)\s*=\s*(.*)$)");

        std::string pendingDescription;
        std::string currentSectionOverride;
        bool haveSectionOverride = false;

        std::istringstream lines(text);
        std::string rawLine;
        std::size_t lineNumber = 0;

        while (std::getline(lines, rawLine)) {
            ++lineNumber;

            const std::string line = trim(rawLine);

            if (line.empty()) {
                continue;
            }

            if (line.front() == '[' && line.back() == ']' && line.size() >= 2) {
                currentSectionOverride = line.substr(1, line.size() - 2);
                haveSectionOverride = true;
                pendingDescription.clear();
                continue;
            }

            if (line.rfind("###", 0) == 0 || line.rfind("##", 0) == 0) {
                // Section/group banner comments (e.g. "### Configuration for instance 'x'" or
                // "## Options (persistent)"). These are purely informational: the section structure
                // is instead derived from the dotted option names themselves, which is more reliable.
                pendingDescription.clear();
                continue;
            }

            if (line.size() >= 2 && line[0] == '#' && line[1] == '@') {
                // A "#@ ..." structured-metadata line (either a "snodec.meta begin/end <entity>"
                // marker or one line of the pretty-printed JSON body between them). This carries
                // machine-readable metadata, not human-readable description text, so it must never
                // be folded into pendingDescription the way ordinary "# ..." comment lines are.
                // Callers that want the metadata use parseMetaBlocks() on the same input separately;
                // this legacy line-oriented parser only needs to not corrupt descriptions with it.
                continue;
            }

            if (line.front() == '#') {
                std::string rest = line.substr(1);
                if (!rest.empty() && rest.front() == ' ') {
                    rest.erase(0, 1);
                }

                std::smatch match;
                if (std::regex_match(rest, match, keyValuePattern)) {
                    const std::string name = haveSectionOverride ? currentSectionOverride + "." + match[1].str() : match[1].str();
                    applyKeyValue(result.model, name, match[2].str(), false, pendingDescription);
                } else {
                    pendingDescription += pendingDescription.empty() ? rest : "\n" + rest;
                }

                continue;
            }

            std::smatch match;
            if (std::regex_match(line, match, keyValuePattern)) {
                const std::string name = haveSectionOverride ? currentSectionOverride + "." + match[1].str() : match[1].str();
                applyKeyValue(result.model, name, match[2].str(), true, pendingDescription);
            } else {
                result.warnings.push_back("Line " + std::to_string(lineNumber) + ": unrecognized configuration line: '" + rawLine + "'");
            }
        }

        return result;
    }

} // namespace snodec::control
