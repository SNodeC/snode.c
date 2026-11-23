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
#include <cstddef>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include <stdexcept>
#include <utility>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace mcp::server::detail {

    UriTemplate::UriTemplate(const std::string& text) {
        std::string literal;
        bool previousVariable = false;
        if (text.empty() || text.size() > 4096) {
            throw std::invalid_argument("template size must be 1..4096 bytes");
        }

        for (std::size_t position = 0; position < text.size(); ++position) {
            const char c = text[position];
            if (c == '{') {
                const auto end = text.find('}', position + 1);
                const bool expansion = position + 1 < text.size() && text[position + 1] == '+';
                const auto start = position + (expansion ? 2 : 1);
                const auto name = text.substr(start, end == std::string::npos ? 0 : end - start);
                if (end == std::string::npos || previousVariable || variables.size() >= 16 || name.empty() ||
                    !std::all_of(name.begin(), name.end(), [](unsigned char byte) {
                        return std::isalnum(byte) || byte == '_' || byte == '.';
                    })) {
                    throw std::invalid_argument("only separated scalar {var}/{+var} expressions are supported (maximum 16)");
                }

                variables.push_back(name);
                literals.push_back(std::move(literal));
                literal.clear();
                reserved.push_back(expansion);
                previousVariable = true;
                position = end;
            } else {
                if (c == '}' || static_cast<unsigned char>(c) <= 32) {
                    throw std::invalid_argument("invalid URI template literal");
                }

                literal += c;
                previousVariable = false;
            }
        }

        literals.push_back(std::move(literal));
    }

    const std::vector<std::string>& UriTemplate::getVariables() const {
        return variables;
    }

    std::optional<std::map<std::string, std::string>> UriTemplate::match(const std::string& uri) const {
        std::optional<std::map<std::string, std::string>> result;
        if (uri.size() <= 16384 && uri.starts_with(literals.front())) {
            std::map<std::string, std::string> values;
            bool valid = true;
            std::size_t position = literals.front().size();
            for (std::size_t i = 0; i < variables.size() && valid; ++i) {
                const auto& following = literals[i + 1];
                const auto end = i + 1 == variables.size() ? (uri.ends_with(following) ? uri.size() - following.size() : std::string::npos)
                                                           : uri.find(following, position);
                valid = end != std::string::npos && end >= position;
                std::string decoded;
                for (std::size_t j = position; valid && j < end; ++j) {
                    const auto c = static_cast<unsigned char>(uri[j]);
                    if (c == '%') {
                        valid = j + 2 < end && std::isxdigit(static_cast<unsigned char>(uri[j + 1])) &&
                                std::isxdigit(static_cast<unsigned char>(uri[j + 2]));
                        if (valid) {
                            decoded += static_cast<char>(std::stoi(uri.substr(j + 1, 2), nullptr, 16));
                            j += 2;
                        }
                    } else {
                        valid = (c < 128 && std::isalnum(c)) || std::string("-._~").find(static_cast<char>(c)) != std::string::npos ||
                                (reserved[i] && std::string(":/?#[]@!$&'()*+,;=").find(static_cast<char>(c)) != std::string::npos);
                        decoded += static_cast<char>(c);
                    }
                }

                if (valid) {
                    try {
                        static_cast<void>(nlohmann::json(decoded).dump());
                        valid = !values.contains(variables[i]) || values.at(variables[i]) == decoded;
                        values[variables[i]] = std::move(decoded);
                        position = end + following.size();
                    } catch (const nlohmann::json::exception&) {
                        valid = false;
                    }
                }
            }

            if (valid && position == uri.size()) {
                result = std::move(values);
            }
        }

        return result;
    }

} // namespace mcp::server::detail
