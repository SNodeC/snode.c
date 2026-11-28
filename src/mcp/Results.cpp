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

#include "mcp/Results.h"

#include "mcp/detail/Serialization.h"

#include <map>
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>
// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>
#include <cmath>
#include <stdexcept>
#include <type_traits>
#include <utility>
#endif
namespace mcp {
    namespace detail {

        Json encode(const Annotations& annotations) {
            Json value = Json::object();
            if (!annotations.audience.empty()) {
                value["audience"] = Json::array();
                for (const auto role : annotations.audience) {
                    value["audience"].push_back(role == Role::User ? "user" : "assistant");
                }
            }

            if (annotations.priority) {
                if (!std::isfinite(*annotations.priority) || *annotations.priority < 0 || *annotations.priority > 1) {
                    throw std::invalid_argument("annotation priority must be in [0,1]");
                }

                value["priority"] = *annotations.priority;
            }

            if (annotations.lastModified) {
                value["lastModified"] = *annotations.lastModified;
            }

            return value;
        }

        void metadata(Json& value, const Json& meta) {
            if (!meta.is_null()) {
                if (!meta.is_object()) {
                    throw std::invalid_argument("metadata must be an object");
                }

                value["_meta"] = meta;
            }
        }

        void display(Json& value, const std::optional<std::string>& title, const std::vector<Icon>& icons) {
            if (title) {
                value["title"] = *title;
            }

            if (!icons.empty()) {
                value["icons"] = Json::array();
                for (const auto& icon : icons) {
                    Json item{{"src", icon.src}};
                    if (icon.mimeType) {
                        item["mimeType"] = *icon.mimeType;
                    }

                    if (!icon.sizes.empty()) {
                        item["sizes"] = icon.sizes;
                    }

                    if (icon.theme) {
                        item["theme"] = *icon.theme == IconTheme::Light ? "light" : "dark";
                    }

                    value["icons"].push_back(std::move(item));
                }
            }
        }

        Json encode(const ResourceContents& contents) {
            return std::visit(
                [](const auto& resource) -> Json {
                    Json value{{"uri", resource.uri}, {"mimeType", resource.mimeType}};
                    metadata(value, resource.meta);
                    if constexpr (std::is_same_v<std::decay_t<decltype(resource)>, TextResource>) {
                        value["text"] = resource.text;
                    } else {
                        value["blob"] = resource.base64;
                    }

                    return value;
                },
                contents);
        }

        Json encode(const Content& content) {
            return std::visit(
                [](const auto& block) -> Json {
                    using Block = std::decay_t<decltype(block)>;
                    Json value;
                    if constexpr (std::is_same_v<Block, TextContent>) {
                        value = {{"type", "text"}, {"text", block.text}};
                    } else if constexpr (std::is_same_v<Block, EmbeddedResource>) {
                        value = {{"type", "resource"}, {"resource", encode(block.resource)}};
                    } else if constexpr (std::is_same_v<Block, ResourceLink>) {
                        value = {{"type", "resource_link"},
                                 {"uri", block.uri},
                                 {"name", block.name},
                                 {"description", block.description},
                                 {"mimeType", block.mimeType}};
                        display(value, block.title, block.icons);
                        if (block.size) {
                            value["size"] = *block.size;
                        }
                    } else {
                        value = {{"type", std::is_same_v<Block, ImageContent> ? "image" : "audio"},
                                 {"data", block.base64},
                                 {"mimeType", block.mimeType}};
                    }

                    const auto annotations = encode(block.annotations);
                    if (!annotations.empty()) {
                        value["annotations"] = annotations;
                    }

                    metadata(value, block.meta);
                    return value;
                },
                content);
        }

    } // namespace detail

    using namespace detail;

    ToolResult::ToolResult(Json value)
        : value(std::move(value)) {
    }

    ToolResult ToolResult::text(std::string text) {
        return content({TextContent{std::move(text), {}}});
    }

    ToolResult ToolResult::json(Json value) {
        ToolResult result = text(value.dump());
        result.value["structuredContent"] = std::move(value);

        return result;
    }

    ToolResult ToolResult::content(std::vector<Content> content) {
        Json blocks = Json::array();
        for (const auto& block : content) {
            blocks.push_back(encode(block));
        }

        return ToolResult(Json{{"content", std::move(blocks)}});
    }

    Json ToolResult::toJson() const {
        return value;
    }

    ToolResult& ToolResult::setMeta(Json meta) {
        metadata(value, meta);

        return *this;
    }

    ToolResult ToolResult::toolError(std::string message) {
        return toolError({TextContent{std::move(message), {}}});
    }

    ToolResult ToolResult::toolError(std::vector<Content> content) {
        ToolResult result = ToolResult::content(std::move(content));
        result.value["isError"] = true;

        return result;
    }

} // namespace mcp
