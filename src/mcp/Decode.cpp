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
#include "mcp/Types.h"
#include "mcp/detail/Serialization.h"

#include <map>
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <vector>
// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#endif
namespace mcp::detail {
    bool validProgress(const Progress& progress, const std::optional<double>& previous) {
        return std::isfinite(progress.value) && progress.value >= 0 && (!previous || progress.value > *previous) &&
               (!progress.total || (std::isfinite(*progress.total) && *progress.total >= progress.value));
    }

    bool validProgressToken(const Json& token) {
        return token.is_string() || (token.is_number_integer() && (!token.is_number_unsigned() || token.get<std::uint64_t>() <= INT64_MAX));
    }

    Progress decodeProgress(const Json& params) {
        return Progress{params.at("progress").get<double>(),
                        params.contains("total") ? std::optional<double>(params.at("total").get<double>()) : std::nullopt,
                        params.value("message", std::string())};
    }

    Json encodeProgress(const Progress& progress, Json token) {
        Json params{{"progress", progress.value}, {"progressToken", std::move(token)}};
        if (progress.total) {
            params["total"] = *progress.total;
        }
        if (!progress.message.empty()) {
            params["message"] = progress.message;
        }
        return params;
    }

    namespace {
        Annotations annotations(const Json& value) {
            Annotations result;
            const auto input = value.value("annotations", Json::object());
            if (input.contains("audience")) {
                for (const auto& role : input["audience"]) {
                    if (role != "user" && role != "assistant") {
                        throw std::invalid_argument("Invalid content audience");
                    }
                    result.audience.push_back(role == "user" ? Role::User : Role::Assistant);
                }
            }
            if (input.contains("priority")) {
                result.priority = input["priority"].get<double>();
            }
            if (input.contains("lastModified")) {
                result.lastModified = input["lastModified"].get<std::string>();
            }
            return result;
        }
    } // namespace

    ResourceContents decodeResource(const Json& value) {
        ResourceContents result;
        const auto uri = value.at("uri").get<std::string>();
        const auto mime = value.value("mimeType", std::string());
        if (value.contains("text")) {
            result = TextResource{uri, mime, value.at("text").get<std::string>(), value.value("_meta", Json())};
        } else {
            result = BlobResource{uri, mime, value.at("blob").get<std::string>(), value.value("_meta", Json())};
        }
        return result;
    }

    Content decodeContent(const Json& value) {
        Content result;
        const auto type = value.at("type").get<std::string>();
        const auto notes = annotations(value);
        const auto meta = value.value("_meta", Json());
        if (type == "text") {
            result = TextContent{value.at("text").get<std::string>(), notes, meta};
        } else if (type == "image") {
            result = ImageContent{value.at("data").get<std::string>(), value.at("mimeType").get<std::string>(), notes, meta};
        } else if (type == "audio") {
            result = AudioContent{value.at("data").get<std::string>(), value.at("mimeType").get<std::string>(), notes, meta};
        } else if (type == "resource") {
            result = EmbeddedResource{decodeResource(value.at("resource")), notes, meta};
        } else if (type == "resource_link") {
            ResourceLink link{value.at("uri").get<std::string>(),
                              value.at("name").get<std::string>(),
                              value.value("description", std::string()),
                              value.value("mimeType", std::string()),
                              {},
                              notes,
                              meta};
            if (value.contains("size")) {
                link.size = value["size"].get<std::uint64_t>();
            }
            if (value.contains("title")) {
                link.title = value["title"].get<std::string>();
            }
            for (const auto& item : value.value("icons", Json::array())) {
                Icon icon{item.at("src").get<std::string>()};
                if (item.contains("mimeType")) {
                    icon.mimeType = item["mimeType"].get<std::string>();
                }
                if (item.contains("sizes")) {
                    icon.sizes = item["sizes"].get<std::vector<std::string>>();
                }
                if (item.contains("theme")) {
                    icon.theme = item["theme"] == "light" ? IconTheme::Light : IconTheme::Dark;
                }
                link.icons.push_back(std::move(icon));
            }
            result = std::move(link);
        } else {
            throw std::invalid_argument("Unsupported content type");
        }
        return result;
    }

    ResourceResult decodeResourceResult(const Json& value) {
        ResourceResult result;
        for (const auto& resource : value.at("contents")) {
            result.contents.push_back(decodeResource(resource));
        }
        result.meta = value.value("_meta", Json());
        if (value.contains("ttlMs")) {
            result.cache = CacheHints{value["ttlMs"].get<std::uint64_t>(),
                                      value.value("cacheScope", "private") == "public" ? CacheScope::Public : CacheScope::Private};
        }
        return result;
    }

    PromptResult decodePromptResult(const Json& value) {
        PromptResult result;
        if (value.contains("description")) {
            result.description = value["description"].get<std::string>();
        }
        for (const auto& message : value.at("messages")) {
            const auto role = message.at("role");
            if (role != "user" && role != "assistant") {
                throw std::invalid_argument("Invalid prompt role");
            }
            result.messages.push_back(PromptMessage{role == "user" ? Role::User : Role::Assistant, decodeContent(message.at("content"))});
        }
        result.meta = value.value("_meta", Json());
        return result;
    }

    CompletionResult decodeCompletionResult(const Json& value) {
        CompletionResult result;
        const auto& completion = value.at("completion");
        result.values = completion.at("values").get<std::vector<std::string>>();
        if (result.values.size() > 100) {
            throw std::invalid_argument("Completion limit exceeded");
        }
        if (completion.contains("total")) {
            result.total = completion["total"].get<std::size_t>();
        }
        result.hasMore = completion.value("hasMore", false);
        result.meta = value.value("_meta", Json());
        return result;
    }

    Json implementation(const Implementation& description) {
        Json value{{"name", description.name}, {"version", description.version}};
        display(value, description.title, description.icons);
        if (description.description) {
            value["description"] = *description.description;
        }
        if (description.websiteUrl) {
            value["websiteUrl"] = *description.websiteUrl;
        }
        return value;
    }
} // namespace mcp::detail

namespace mcp {
    ToolResult ToolResult::fromJson(const Json& value) {
        if (!value.is_object() || !value.value("content", Json::array()).is_array()) {
            throw std::invalid_argument("Invalid tool result");
        }
        for (const auto& block : value.value("content", Json::array())) {
            static_cast<void>(detail::decodeContent(block));
        }
        if (value.contains("isError")) {
            static_cast<void>(value["isError"].get<bool>());
        }
        return ToolResult(value);
    }
} // namespace mcp
