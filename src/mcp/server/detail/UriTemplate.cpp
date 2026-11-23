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

#include "mcp/server/detail/UriTemplate.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>
#include <stdexcept>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mcp::server::detail {

    UriTemplate::UriTemplate(const std::string& text) {
        std::string pattern = "^";
        bool previousVariable = false;
        if (text.empty() || text.size() > 4096) {
            throw std::invalid_argument("template size must be 1..4096 bytes");
        }

        for (std::size_t position = 0; position < text.size(); ++position) {
            const char c = text[position];
            if (c == '{') {
                const auto end = text.find('}', position + 1);
                const bool reserved = position + 1 < text.size() && text[position + 1] == '+';
                const auto start = position + (reserved ? 2 : 1);
                const auto name = text.substr(start, end == std::string::npos ? 0 : end - start);
                if (end == std::string::npos || previousVariable || variables.size() >= 16 || name.empty() ||
                    !std::all_of(name.begin(), name.end(), [](unsigned char byte) {
                        return std::isalnum(byte) || byte == '_' || byte == '.';
                    })) {
                    throw std::invalid_argument("only separated scalar {var}/{+var} expressions are supported (maximum 16)");
                }

                variables.push_back(name);
                pattern += reserved ? "(.*?)" : "([^/?#]*?)";
                previousVariable = true;
                position = end;
            } else {
                if (c == '}' || static_cast<unsigned char>(c) <= 32) {
                    throw std::invalid_argument("invalid URI template literal");
                }

                if (std::string(".\\^$|()[]*+?{}").find(c) != std::string::npos) {
                    pattern += '\\';
                }

                pattern += c;
                previousVariable = false;
            }
        }

        expression = std::regex(pattern + "$", std::regex::ECMAScript);
    }

    const std::vector<std::string>& UriTemplate::getVariables() const {
        return variables;
    }

    std::optional<std::map<std::string, std::string>> UriTemplate::match(const std::string& uri) const {
        std::optional<std::map<std::string, std::string>> result;
        std::smatch captures;
        if (uri.size() <= 16384 && std::regex_match(uri, captures, expression)) {
            std::map<std::string, std::string> values;
            bool valid = true;
            for (std::size_t i = 0; i < variables.size() && valid; ++i) {
                const auto raw = captures[i + 1].str();
                std::string decoded;
                for (std::size_t j = 0; j < raw.size() && valid; ++j) {
                    if (raw[j] == '%') {
                        valid = j + 2 < raw.size() && std::isxdigit(static_cast<unsigned char>(raw[j + 1])) &&
                                std::isxdigit(static_cast<unsigned char>(raw[j + 2]));
                        if (valid) {
                            decoded += static_cast<char>(std::stoi(raw.substr(j + 1, 2), nullptr, 16));
                            j += 2;
                        }
                    } else {
                        decoded += raw[j];
                    }
                }

                if (valid) {
                    try {
                        static_cast<void>(nlohmann::json(decoded).dump());
                        valid = !values.contains(variables[i]) || values.at(variables[i]) == decoded;
                        values[variables[i]] = std::move(decoded);
                    } catch (const nlohmann::json::exception&) {
                        valid = false;
                    }
                }
            }

            if (valid) {
                result = std::move(values);
            }
        }

        return result;
    }

} // namespace mcp::server::detail
