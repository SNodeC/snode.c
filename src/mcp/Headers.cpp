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

#include "mcp/detail/Headers.h"

#include "utils/base64.h"

#include <cmath>
#ifndef DOXYGEN_SHOULD_SKIP_THIS
// IWYU pragma: no_include <nlohmann/detail/iterators/iter_impl.hpp>
// IWYU pragma: no_include <nlohmann/detail/iterators/iteration_proxy.hpp>
#include <algorithm>
#include <functional>
#include <nlohmann/json.hpp>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>
#endif
namespace mcp::detail {
    std::optional<HeaderParameters> headerParameters(const Json& schema) {
        HeaderParameters result;
        std::set<std::string> names;
        bool valid = true;
        const std::function<void(const Json&, const std::vector<std::string>&, bool)> walk =
            [&](const Json& node, const std::vector<std::string>& path, bool reachable) {
                if (node.is_object()) {
                    if (node.contains("x-mcp-header")) {
                        const Json annotation = node["x-mcp-header"];
                        const Json type = node.value("type", Json());
                        valid = valid && reachable && !path.empty() && annotation.is_string() &&
                                (type == "string" || type == "integer" || type == "boolean");
                        if (annotation.is_string()) {
                            const std::string name = annotation.get<std::string>();
                            std::string lower = name;
                            for (char& c : lower) {
                                if (c >= 'A' && c <= 'Z') {
                                    c = static_cast<char>(c + ('a' - 'A'));
                                }
                            }
                            valid = valid && !name.empty() &&
                                    std::all_of(name.begin(),
                                                name.end(),
                                                [](unsigned char c) {
                                                    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                                                           std::string_view("!#$%&'*+-.^_`|~").find(static_cast<char>(c)) !=
                                                               std::string_view::npos;
                                                }) &&
                                    names.insert(lower).second;
                            result.emplace(path, name);
                        }
                    }
                    for (const auto& [key, value] : node.items()) {
                        if (reachable && key == "properties" && value.is_object()) {
                            for (const auto& [property, child] : value.items()) {
                                auto next = path;
                                next.push_back(property);
                                walk(child, next, true);
                            }
                        } else if (key != "x-mcp-header") {
                            walk(value, path, false);
                        }
                    }
                } else if (node.is_array()) {
                    for (const auto& child : node) {
                        walk(child, path, false);
                    }
                }
            };
        walk(schema, {}, true);
        return valid ? std::optional<HeaderParameters>(std::move(result)) : std::nullopt;
    }

    std::string encodeHeader(const Json& value) {
        const std::string text = value.is_string() ? value.get<std::string>() : value.dump();
        const bool plain = !text.empty() && text.front() != ' ' && text.back() != ' ' &&
                           !(text.starts_with("=?base64?") && text.ends_with("?=")) &&
                           std::all_of(text.begin(), text.end(), [](unsigned char c) {
                               return c >= 32 && c <= 126;
                           });
        return plain ? text : "=?base64?" + base64::base64_encode(reinterpret_cast<const unsigned char*>(text.data()), text.size()) + "?=";
    }

    std::optional<std::string> decodeHeader(const std::string& value) {
        std::optional<std::string> decoded;
        if (!value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char c) {
                return c == 9 || (c >= 32 && c <= 126);
            })) {
            if (value.starts_with("=?base64?") && value.ends_with("?=")) {
                const auto payload = value.substr(9, value.size() - 11);
                const auto text = base64::base64_decode(payload);
                if (base64::base64_encode(reinterpret_cast<const unsigned char*>(text.data()), text.size()) == payload) {
                    decoded = text;
                }
            } else {
                decoded = value;
            }
        }
        return decoded;
    }

    std::map<std::string, std::string> parameterHeaders(const HeaderParameters& parameters, const Json& arguments) {
        std::map<std::string, std::string> headers;
        for (const auto& [path, name] : parameters) {
            const Json* value = &arguments;
            for (const auto& key : path) {
                value = value != nullptr && value->is_object() && value->contains(key) ? &value->at(key) : nullptr;
            }
            if (value != nullptr) {
                if (!(value->is_string() || value->is_boolean() || value->is_number_integer()) ||
                    (value->is_number_integer() &&
                     (value->get<double>() < -9007199254740991.0 || value->get<double>() > 9007199254740991.0))) {
                    throw std::invalid_argument("Unsafe MCP header argument");
                }
                headers.emplace("Mcp-Param-" + name, encodeHeader(*value));
            }
        }
        return headers;
    }
} // namespace mcp::detail
