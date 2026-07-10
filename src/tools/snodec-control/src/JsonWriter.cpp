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

#include "JsonWriter.h"

#include <cstdio>
#include <sstream>

namespace snodec::control {

    namespace {

        void appendOptionalString(std::ostringstream& out, const char* name, const std::optional<std::string>& value) {
            out << "\"" << name << "\": ";
            if (value.has_value()) {
                out << "\"" << jsonEscape(*value) << "\"";
            } else {
                out << "null";
            }
        }

    } // namespace

    std::string jsonEscape(const std::string& value) {
        std::string escaped;
        escaped.reserve(value.size());

        for (const char character : value) {
            switch (character) {
                case '"':
                    escaped += "\\\"";
                    break;
                case '\\':
                    escaped += "\\\\";
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
                    if (static_cast<unsigned char>(character) < 0x20) {
                        char buffer[8];
                        std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned int>(static_cast<unsigned char>(character)));
                        escaped += buffer;
                    } else {
                        escaped.push_back(character);
                    }
                    break;
            }
        }

        return escaped;
    }

    std::string toJson(const ConfigModel& model, const std::string& targetPath) {
        std::ostringstream out;

        out << "{\n";
        out << "  \"target\": \"" << jsonEscape(targetPath) << "\",\n";
        out << "  \"sectionCount\": " << model.sectionCount() << ",\n";
        out << "  \"optionCount\": " << model.optionCount() << ",\n";
        out << "  \"requiredOptionCount\": " << model.requiredOptionCount() << ",\n";
        out << "  \"activeOptionCount\": " << model.activeOptionCount() << ",\n";
        out << "  \"sections\": [\n";

        for (std::size_t sectionIndex = 0; sectionIndex < model.sections.size(); ++sectionIndex) {
            const ConfigSection& section = model.sections[sectionIndex];

            out << "    {\n";
            out << "      \"name\": \"" << jsonEscape(section.name) << "\",\n";
            out << "      \"options\": [\n";

            for (std::size_t optionIndex = 0; optionIndex < section.options.size(); ++optionIndex) {
                const ConfigOption& option = section.options[optionIndex];

                out << "        {\n";
                out << "          \"key\": \"" << jsonEscape(option.key) << "\",\n";
                out << "          \"description\": \"" << jsonEscape(option.description) << "\",\n";
                out << "          ";
                appendOptionalString(out, "defaultValue", option.defaultValue);
                out << ",\n";
                out << "          ";
                appendOptionalString(out, "activeValue", option.activeValue);
                out << ",\n";
                out << "          \"required\": " << (option.required ? "true" : "false") << ",\n";
                out << "          \"hasActiveValue\": " << (option.hasActiveValue ? "true" : "false") << "\n";
                out << "        }" << (optionIndex + 1 < section.options.size() ? "," : "") << "\n";
            }

            out << "      ]\n";
            out << "    }" << (sectionIndex + 1 < model.sections.size() ? "," : "") << "\n";
        }

        out << "  ]\n";
        out << "}\n";

        return out.str();
    }

} // namespace snodec::control
